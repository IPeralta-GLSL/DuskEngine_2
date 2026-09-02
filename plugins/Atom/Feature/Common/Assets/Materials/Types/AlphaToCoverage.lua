function GetMaterialPropertyDependencies()
    return {"alphaToCoverage"}
end

function Process(context)
    local enabled = context:GetMaterialPropertyValue_bool("alphaToCoverage")
    local lastShader = context:GetShaderCount() - 1

    if enabled then
        for i = 0, lastShader do
            context:GetShader(i):GetRenderStatesOverride():SetAlphaToCoverageEnabled(true)
        end
    else
        for i = 0, lastShader do
            context:GetShader(i):GetRenderStatesOverride():ClearAlphaToCoverageEnabled()
        end
    end
end
