/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzFramework/ProjectManager/ProjectManager.h>

#if defined(AZ_PLATFORM_LINUX)
#include <signal.h>
#include <execinfo.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <atomic>

// ── Linux Crash Handler + Freeze Watchdog ────────────────────────────────────
//
//  • On SIGSEGV/SIGABRT/SIGFPE/SIGILL/SIGBUS: writes crash log + backrace to
//    /tmp/o3de_crash_<timestamp>.log  and copies Editor.log next to it.
//
//  • Heartbeat thread: writes current epoch second to
//    /tmp/o3de_editor_heartbeat_<pid> every second.
//    The external watchdog script reads this file to detect freezes.
//
// ─────────────────────────────────────────────────────────────────────────────

namespace LinuxCrashHandler
{
    static char  s_projectPath[512]   = {};
    static char  s_heartbeatPath[64]  = {};
    static std::atomic<bool> s_heartbeatRunning{ false };

    static void fdWrite(int fd, const char* str)
    {
        if (str && *str) (void)write(fd, str, strlen(str));
    }

    static const char* SignalName(int sig)
    {
        switch (sig)
        {
            case SIGSEGV: return "SIGSEGV - Segmentation Fault";
            case SIGABRT: return "SIGABRT - Abort";
            case SIGFPE:  return "SIGFPE  - Floating Point Exception";
            case SIGILL:  return "SIGILL  - Illegal Instruction";
            case SIGBUS:  return "SIGBUS  - Bus Error";
            default:      return "Unknown Signal";
        }
    }

    static struct sigaction s_oldHandlers[32];

    static void CrashSignalHandler(int sig, siginfo_t* /*info*/, void* /*ctx*/)
    {
        // Build crash log path: /tmp/o3de_crash_<timestamp>.log
        char crashPath[256];
        {
            time_t now = time(nullptr);
            struct tm tm_info;
            localtime_r(&now, &tm_info);
            char ts[32];
            strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm_info);
            snprintf(crashPath, sizeof(crashPath), "/tmp/o3de_crash_%s.log", ts);
        }

        int fd = open(crashPath, O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (fd < 0) fd = STDERR_FILENO;

        fdWrite(fd, "================================================\n");
        fdWrite(fd, "  O3DE LINUX CRASH REPORT\n");
        fdWrite(fd, "================================================\n");
        fdWrite(fd, "Signal : "); fdWrite(fd, SignalName(sig)); fdWrite(fd, "\n");
        {
            time_t now = time(nullptr);
            char ts[64]; ctime_r(&now, ts);
            fdWrite(fd, "Time   : "); fdWrite(fd, ts);
        }
        fdWrite(fd, "\n── Stack Trace ─────────────────────────────────\n");
        void* frames[128];
        int n = backtrace(frames, 128);
        backtrace_symbols_fd(frames, n, fd);
        fdWrite(fd, "────────────────────────────────────────────────\n");

        // Copy Editor.log to /tmp/
        if (s_projectPath[0] != '\0')
        {
            char editorLog[600];
            snprintf(editorLog, sizeof(editorLog), "%s/user/log/Editor.log", s_projectPath);
            char ts[32]; time_t now = time(nullptr); struct tm tm_info;
            localtime_r(&now, &tm_info);
            strftime(ts, sizeof(ts), "%Y%m%d_%H%M%S", &tm_info);
            char copyPath[256];
            snprintf(copyPath, sizeof(copyPath), "/tmp/o3de_editor_at_crash_%s.log", ts);
            int src = open(editorLog, O_RDONLY);
            int dst = open(copyPath, O_WRONLY | O_CREAT | O_TRUNC, 0664);
            if (src >= 0 && dst >= 0)
            {
                char buf[4096]; ssize_t r;
                while ((r = read(src, buf, sizeof(buf))) > 0) (void)write(dst, buf, r);
                char msg[512];
                snprintf(msg, sizeof(msg), "\n(Editor.log copied from: %s)\n", editorLog);
                fdWrite(fd, msg);
            }
            if (src >= 0) close(src);
            if (dst >= 0) close(dst);
        }

        if (fd != STDERR_FILENO) close(fd);

        // Also print crash path to stderr
        char notice[300];
        snprintf(notice, sizeof(notice), "\n[O3DE] Crash log saved to: %s\n", crashPath);
        (void)write(STDERR_FILENO, notice, strlen(notice));

        // Remove heartbeat so watchdog knows we crashed
        if (s_heartbeatPath[0] != '\0') unlink(s_heartbeatPath);

        // Re-raise to produce core dump / default handling
        sigaction(sig, &s_oldHandlers[sig], nullptr);
        raise(sig);
    }

    static void* HeartbeatThread(void*)
    {
        while (s_heartbeatRunning.load())
        {
            int fd = open(s_heartbeatPath, O_WRONLY | O_CREAT | O_TRUNC, 0664);
            if (fd >= 0)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%lld\n", (long long)time(nullptr));
                (void)write(fd, buf, strlen(buf));
                close(fd);
            }
            sleep(1);
        }
        if (s_heartbeatPath[0] != '\0') unlink(s_heartbeatPath);
        return nullptr;
    }

    static void Install(const char* projectPath)
    {
        if (projectPath) snprintf(s_projectPath, sizeof(s_projectPath), "%s", projectPath);

        snprintf(s_heartbeatPath, sizeof(s_heartbeatPath), "/tmp/o3de_editor_heartbeat_%d", (int)getpid());

        // Signal handlers
        static const int kSignals[] = { SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS };
        struct sigaction sa{};
        sa.sa_sigaction = CrashSignalHandler;
        sa.sa_flags     = SA_SIGINFO | SA_RESETHAND;
        sigemptyset(&sa.sa_mask);
        for (int sig : kSignals) sigaction(sig, &sa, &s_oldHandlers[sig]);

        // Heartbeat thread
        s_heartbeatRunning = true;
        pthread_t tid;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&tid, &attr, HeartbeatThread, nullptr);
        pthread_attr_destroy(&attr);

        fprintf(stderr, "[O3DE] Linux crash handler active. Heartbeat: %s\n", s_heartbeatPath);
    }

    static void Shutdown()
    {
        s_heartbeatRunning = false;
    }

} // namespace LinuxCrashHandler
// ─────────────────────────────────────────────────────────────────────────────
#endif // AZ_PLATFORM_LINUX

int main(int argc, char* argv[])
{
#if defined(AZ_PLATFORM_LINUX)
    signal(SIGHUP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    // Find --project-path argument for crash log copy
    const char* projectPath = nullptr;
    for (int i = 1; i < argc - 1; ++i)
    {
        if (strcmp(argv[i], "--project-path") == 0 || strcmp(argv[i], "-project-path") == 0)
        {
            projectPath = argv[i + 1];
            break;
        }
    }
    LinuxCrashHandler::Install(projectPath);
#endif
    const AZ::Debug::Trace tracer;
    // Verify a project path can be found, launch the project manager and shut down otherwise
    if (AzFramework::ProjectManager::CheckProjectPathProvided(argc, argv) == AzFramework::ProjectManager::ProjectPathCheckResult::ProjectManagerLaunched)
    {
        return 2;
    }
    using CryEditMain = int (*)(int, char*[]);
    constexpr const char CryEditMainName[] = "CryEditMain";

    auto handle = AZ::DynamicModuleHandle::Create("EditorLib");
    [[maybe_unused]] const bool loaded = handle->Load(AZ::DynamicModuleHandle::LoadFlags::InitFuncRequired);
    AZ_Assert(loaded, "EditorLib could not be loaded");

    int ret = 1;
    if (auto fn = handle->GetFunction<CryEditMain>(CryEditMainName); fn != nullptr)
    {
        ret = AZStd::invoke(fn, argc, argv);
    }

    handle = {};
#if defined(AZ_PLATFORM_LINUX)
    LinuxCrashHandler::Shutdown();
#endif
    return ret;
}
