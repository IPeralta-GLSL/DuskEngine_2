{
    "Source": "WireframeDebugOverlay.azsl",

    "DepthStencilState": {
        "Depth": { "Enable": false },
        "Stencil": { "Enable": false }
    },

    "GlobalTargetBlendState": {
        "Enable": true,
        "BlendSource": "One",
        "BlendDest": "One",
        "BlendOp": "Add",
        "BlendAlphaSource": "Zero",
        "BlendAlphaDest": "One",
        "BlendAlphaOp": "Add"
    },

    "ProgramSettings":
    {
        "EntryPoints":
        [
            {
                "name": "MainVS",
                "type": "Vertex"
            },
            {
                "name": "MainPS",
                "type": "Fragment"
            }
        ]
    }
}
