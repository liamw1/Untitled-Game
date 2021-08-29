#include "ENpch.h"
#include "ImGuiLayer.h"
#include "Platform/OpenGL/ImGuiOpenGLRenderer.h"
#include "Engine/Core/Application.h"

// TEMPORARY
#include <GLFW/glfw3.h>
#include <glad/glad.h>


namespace Engine
{
  ImGuiLayer::ImGuiLayer()
    : Layer("ImGuiLayer")
  {
  }

  ImGuiLayer::~ImGuiLayer()
  {
  }

  void ImGuiLayer::onAttach()
  {
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;

    // TEMPORARY: shoud eventually use custom key codes
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    ImGui_ImplOpenGL3_Init("#version 410");
  }

  void ImGuiLayer::onDetach()
  {
  }

  void ImGuiLayer::onUpdate()
  {
    ImGuiIO& io = ImGui::GetIO();
    Application& app = Application::Get();
    io.DisplaySize = ImVec2((float)app.getWindow().getWidth(), (float)app.getWindow().getHeight());

    float time = (float)glfwGetTime();
    io.DeltaTime = m_Time > 0.0 ? (time - m_Time) : (1.0f / 60.0f);
    m_Time = time;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();

    static bool show = true;
    ImGui::ShowDemoWindow(&show);

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  }

  void ImGuiLayer::onEvent(Event& event)
  {
    EventDispatcher dispatcher(event);
    dispatcher.dispatch<MouseButtonPressEvent>(EN_BIND_EVENT_FN(onMouseButtonPressEvent));
    dispatcher.dispatch<MouseButtonReleaseEvent>(EN_BIND_EVENT_FN(onMouseButtonReleaseEvent));
    dispatcher.dispatch<MouseMoveEvent>(EN_BIND_EVENT_FN(onMouseMoveEvent));
    dispatcher.dispatch<MouseScrollEvent>(EN_BIND_EVENT_FN(onMouseScrollEvent));
    dispatcher.dispatch<KeyPressEvent>(EN_BIND_EVENT_FN(onKeyPressEvent));
    dispatcher.dispatch<KeyTypeEvent>(EN_BIND_EVENT_FN(onKeyTypeEvent));
    dispatcher.dispatch<KeyReleaseEvent>(EN_BIND_EVENT_FN(onKeyReleaseEvent));
    dispatcher.dispatch<WindowResizeEvent>(EN_BIND_EVENT_FN(onWindowResizeEvent));
  }

  bool ImGuiLayer::onMouseButtonPressEvent(MouseButtonPressEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.getMouseButton()] = true;
    return false;
  }

  bool ImGuiLayer::onMouseButtonReleaseEvent(MouseButtonReleaseEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.getMouseButton()] = false;
    return false;
  }

  bool ImGuiLayer::onMouseMoveEvent(MouseMoveEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.MousePos = ImVec2(event.getX(), event.getY());
    return false;
  }

  bool ImGuiLayer::onMouseScrollEvent(MouseScrollEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += event.getXOffset();
    io.MouseWheel += event.getYOffset();
    return false;
  }

  bool ImGuiLayer::onKeyPressEvent(KeyPressEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.getKeyCode()] = true;
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
    return false;
  }

  bool ImGuiLayer::onKeyReleaseEvent(KeyReleaseEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseDown[event.getKeyCode()] = false;
    return false;
  }

  bool ImGuiLayer::onKeyTypeEvent(KeyTypeEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    int keycode = event.getKeyCode();
    if (keycode > 0 && keycode < 0x10000)
      io.AddInputCharacter((unsigned short)keycode);
    return false;
  }

  bool ImGuiLayer::onWindowResizeEvent(WindowResizeEvent& event)
  {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)event.getWidth(), (float)event.getHeight());
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
    glViewport(0, 0, event.getWidth(), event.getHeight());
    return false;
  }
}