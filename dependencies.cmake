include(FetchContent)

# Set up SDL3 dependency
FetchContent_Declare(SDL3
        GIT_REPOSITORY  https://github.com/libsdl-org/SDL.git
        GIT_TAG         release-3.4.x
        GIT_SHALLOW     TRUE
        OVERRIDE_FIND_PACKAGE
)

# Set up SPDLOG dependency
FetchContent_Declare(spdlog
        GIT_REPOSITORY  https://github.com/gabime/spdlog.git
        GIT_TAG         v1.17.0
        GIT_SHALLOW     TRUE
        OVERRIDE_FIND_PACKAGE
)

# Set up Volk dependency
FetchContent_Declare(volk
        GIT_REPOSITORY  https://github.com/zeux/volk.git
        GIT_TAG         master
        GIT_SHALLOW     TRUE
        OVERRIDE_FIND_PACKAGE
)

# Set up VMA dependency
FetchContent_Declare(VulkanMemoryAllocator
        GIT_REPOSITORY  https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG         v3.4.0
        GIT_SHALLOW     TRUE
        OVERRIDE_FIND_PACKAGE
)
