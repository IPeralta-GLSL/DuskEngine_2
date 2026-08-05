/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Debug/Trace.h>
#include <dlfcn.h>

namespace QtForPython
{
    // s_libPythonLibraryFile must match the library name listed in (O3DE Engine Root)/python/runtime/.../python-config.cmake
    // in the set(${MY}_LIBRARY_xxxx sections.
    const char* s_libPythonLibraryFile = "libpython3.10.so.1.0"; 
    const char* s_libPyside6LibraryFile = "libpyside6.abi3.so.6.5";
    const char* s_libShibokenLibraryFile = "libshiboken6.abi3.so.6.5";
    const char* s_libQt6TestLibraryFile = "libQt6Test.so.6";

    class InitializeEmbeddedPyside6
    {
    public:
        InitializeEmbeddedPyside6()
        {
            m_libPythonLibraryFile = InitializeEmbeddedPyside6::LoadModule(s_libPythonLibraryFile);
            m_libPyside6LibraryFile = InitializeEmbeddedPyside6::LoadModule(s_libPyside6LibraryFile);
            m_libShibokenLibraryFile = InitializeEmbeddedPyside6::LoadModule(s_libShibokenLibraryFile);
            m_libQt6TestLibraryFile = InitializeEmbeddedPyside6::LoadModule(s_libQt6TestLibraryFile);
        }
        virtual ~InitializeEmbeddedPyside6()
        {
            InitializeEmbeddedPyside6::UnloadModule(m_libQt6TestLibraryFile);
            InitializeEmbeddedPyside6::UnloadModule(m_libShibokenLibraryFile);
            InitializeEmbeddedPyside6::UnloadModule(m_libPyside6LibraryFile);
            InitializeEmbeddedPyside6::UnloadModule(m_libPythonLibraryFile);
        }

    private:
        static void* LoadModule(const char* moduleToLoad)
        {
            void* moduleHandle = dlopen(moduleToLoad, RTLD_NOW | RTLD_GLOBAL);
            if (!moduleHandle)
            {
                [[maybe_unused]] const char* loadError = dlerror();
                AZ_Error("QtForPython", false, "Unable to load python library %s for Pyside6: %s", moduleToLoad,
                         loadError ? loadError : "Unknown Error");
            }
            return moduleHandle;
        }

        static void UnloadModule(void* moduleHandle)
        {
            if (moduleHandle)
            {
                dlclose(moduleHandle);
            }
        }

        void* m_libPythonLibraryFile;
        void* m_libPyside6LibraryFile;
        void* m_libShibokenLibraryFile;
        void* m_libQt6TestLibraryFile;
    };
} // namespace QtForPython
