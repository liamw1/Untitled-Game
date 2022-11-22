VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"] = "%{wks.location}/Engine/lib/GLFW/include"
IncludeDir["GLad"] = "%{wks.location}/Engine/lib/GLad/include"
IncludeDir["shaderc"] = "%{wks.location}/Engine/lib/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Engine/lib/SPIRV-Cross"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["ImGui"] = "%{wks.location}/Engine/lib/imgui"
IncludeDir["glm"] = "%{wks.location}/Engine/lib/glm"
IncludeDir["stb_image"] = "%{wks.location}/Engine/lib/stb_image"
IncludeDir["spdlog"] = "%{wks.location}/Engine/lib/spdlog/include"
IncludeDir["EnTT"] = "%{wks.location}/Engine/lib/EnTT/single_include"

LibraryDir = {}
LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"
Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"
Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"