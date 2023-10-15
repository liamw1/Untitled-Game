#pragma once
/*
  For use by Engine applications
*/

#include "Engine/Core/Application.h"
#include "Engine/Core/Concepts.h"
#include "Engine/Core/Layer.h"
#include "Engine/Core/Time.h"
#include "Engine/Core/UID.h"
#include "Engine/Core/Input/MouseButtonCodes.h"
#include "Engine/Core/Input/KeyCodes.h"
#include "Engine/Core/Input/Input.h"

#include "Engine/Debug/Assert.h"
#include "Engine/Debug/Instrumentor.h"
#include "Engine/Debug/TestClasses.h"
#include "Engine/Debug/Timer.h"

#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/KeyEvent.h"
#include "Engine/Events/MouseEvent.h"

#include "Engine/ImGui/ImGuiLayer.h"

#include "Engine/Math/ArrayBox.h"
#include "Engine/Math/ArrayRect.h"
#include "Engine/Math/Direction.h"
#include "Engine/Math/IBox2.h"
#include "Engine/Math/IBox3.h"
#include "Engine/Math/IVec2.h"
#include "Engine/Math/IVec3.h"
#include "Engine/Math/Units.h"
#include "Engine/Math/Vec.h"

#include "Engine/Renderer/BufferLayout.h"
#include "Engine/Renderer/Camera.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/MemoryPool.h"
#include "Engine/Renderer/MultiDrawArray.h"
#include "Engine/Renderer/RenderCommand.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/Shader.h"
#include "Engine/Renderer/StorageBuffer.h"
#include "Engine/Renderer/Texture.h"
#include "Engine/Renderer/Uniform.h"
#include "Engine/Renderer/VertexArray.h"

#include "Engine/Scene/Components.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Scene/Scripting.h"

#include "Engine/Threads/ThreadPool.h"
#include "Engine/Threads/Threads.h"
#include "Engine/Threads/WorkSet.h"
#include "Engine/Threads/Containers/LRUCache.h"
#include "Engine/Threads/Containers/ProtectedArrayBox.h"
#include "Engine/Threads/Containers/UnorderedMap.h"
#include "Engine/Threads/Containers/UnorderedSet.h"

#include "Engine/Utilities/BitUtilities.h"
#include "Engine/Utilities/Constraints.h"
#include "Engine/Utilities/EnumUtilities.h"
#include "Engine/Utilities/Helpers.h"
#include "Engine/Utilities/LRUCache.h"
#include "Engine/Utilities/MoveOnlyFunction.h"