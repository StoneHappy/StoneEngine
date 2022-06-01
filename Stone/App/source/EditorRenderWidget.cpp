#include "EditorRenderWidget.h"

#include <iostream>
#include <Core/Base/macro.h>
#include <Engine.h>
#include <Function/Render/Renderer.h>
#include <Function/Event/EventSystem.h>
#include <Function/Scene/EditCamera.h>
#include <Function/Scene/Scene.h>
#include <Resource/Components/Mesh.h>
#include <Resource/Components/Transform.h>
#include <Resource/Data/Implement/VCG/VCGMesh.h>
#include <qelapsedtimer.h>
#include <qevent.h>
#include <QtImGui.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include <Function/Scene/Light.h>
#include <glm/gtc/type_ptr.hpp>

#include <Resource/Components/Model.h>
#include <Resource/Data/Implement/Assimp/AssimpMesh.h>
#include <Resource/Data/Implement/Assimp/AssimpNode.h>
#include <Resource/Data/Interface/ModelPool.h>

#include <Function/Scene/Billboard.h>
#include "Function/Render/Shader.h"
#include "Function/Render/Buffer.h"
#include "Function/Render/VertexArray.h"
#include <test_vert.h>
#include <test_geom.h>

#include <Function/Particle/Particle.h>
namespace Stone
{
    std::shared_ptr<Shader> testshader;
	EditorRendererWidget::EditorRendererWidget(QWidget* parent)
		: QOpenGLWidget(parent), m_MousePos(std::make_shared<MousePos>(0.0f, 0.0f)), m_MouseAngle(std::make_shared<MouseAngle>(0.0f, 0.0f))
	{}
	EditorRendererWidget::~EditorRendererWidget()
	{
    }

	void EditorRendererWidget::initializeGL()
	{
        PublicSingleton<Engine>::getInstance().renderInitialize();
        PublicSingleton<Engine>::getInstance().logicalInitialize();
        QtImGui::initialize(this);
        Particle particle;
        particle.x = 1.0;
        particle.y = 1.0;
        particle.z = 1.0;

        Particle particle1;
        particle1.x = 0.0;
        particle1.y = 0.0;
        particle1.z = 0.0;

        PublicSingleton<ParticleSystem>::getInstance().init();
        PublicSingleton<ParticleSystem>::getInstance().add(particle);
        PublicSingleton<ParticleSystem>::getInstance().add(particle1);
        glPointSize(10);
	}

	void EditorRendererWidget::resizeGL(int w, int h)
	{
        PublicSingleton<Renderer>::getInstance().screenFrameBuffer()->resize(w, h);
        PublicSingleton<EditorCamera>::getInstance().setViewportSize(w, h);
	}

	void EditorRendererWidget::paintGL()
	{
        QElapsedTimer timer;
        timer.start();
        PublicSingleton<Engine>::getInstance().logicalTick();
        PublicSingleton<Renderer>::getInstance().begin();
        PublicSingleton<Scene>::getInstance().renderTick();
        PublicSingleton<ParticleSystem>::getInstance().logictick();
        PublicSingleton<ParticleSystem>::getInstance().rendertick();
        PublicSingleton<Renderer>::getInstance().end(defaultFramebufferObject());
        renderImGui();
        update();
        PublicSingleton<Engine>::getInstance().DeltaTime = timer.nsecsElapsed()* 1.0e-9f;
	}


    void EditorRendererWidget::mousePressEvent(QMouseEvent* event)
    {
        QKeyEvent* e = (QKeyEvent*)event;
        if (e->modifiers() == Qt::ShiftModifier)
        {
            m_MousePos->x = event->pos().x();
            m_MousePos->y = -event->pos().y();
            PublicSingletonInstance(EventSystem).sendEvent("EditCamera_Begin", (void*)m_MousePos.get());
        }
    }

    void EditorRendererWidget::mouseDoubleClickEvent(QMouseEvent* event)
    {
    }

    void EditorRendererWidget::mouseMoveEvent(QMouseEvent* event)
    {
        QKeyEvent* e = (QKeyEvent*)event;
        if (e->modifiers() == Qt::ShiftModifier)
        {
            m_MousePos->x = -event->pos().x();
            m_MousePos->y = event->pos().y();
            if (event->buttons() & Qt::LeftButton)
            {

                PublicSingletonInstance(EventSystem).sendEvent("EditCamera_Rotate", (void*)m_MousePos.get());
            }
            else if (event->buttons() & Qt::MiddleButton)
            {

                PublicSingletonInstance(EventSystem).sendEvent("EditCamera_Pan", (void*)m_MousePos.get());
            }
        }
        
    }

    void EditorRendererWidget::mouseReleaseEvent(QMouseEvent* event)
    {
        QKeyEvent* e = (QKeyEvent*)event;
        if (e->modifiers() == Qt::ShiftModifier)
        {
            PublicSingletonInstance(EventSystem).sendEvent("EditCamera_End", (void*)m_MousePos.get());
        }
    }

    void EditorRendererWidget::wheelEvent(QWheelEvent* event)
    {
        m_MouseAngle->x = event->angleDelta().x();
        m_MouseAngle->y = event->angleDelta().y();
        PublicSingletonInstance(EventSystem).sendEvent("EditCamera_Zoom", (void*)m_MouseAngle.get());
    }

    void EditorRendererWidget::renderImGui()
    {
        QtImGui::newFrame();
        ImGuizmo::BeginFrame();
        ImGuizmo::SetOrthographic(false);
        int x, y, width, height;
        const QRect& geom = this->geometry();
        x = geom.x();
        y = geom.y();
        width = geom.width();
        height = geom.height();
        ImGuizmo::SetRect(x, y, width, height);
        auto& view = PublicSingletonInstance(EditorCamera).getViewMatrix();
        auto& proj = PublicSingletonInstance(EditorCamera).getProjMatrix();
        glm::mat4 testMatrix = glm::mat4(1);
        ImGuizmo::ViewManipulate(glm::value_ptr(view), 8.f, ImVec2(x + width - width * 0.1, 0), ImVec2(width * 0.1, width * 0.1), 0x10101010);
        PublicSingletonInstance(EditorCamera).updateBuffer();
        ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), ImGuizmo::OPERATION::ROTATE, ImGuizmo::LOCAL, glm::value_ptr(testMatrix));
        ImGui::Text("Hello");
        float aaa = 0.0f;
        ImGui::DragFloat3("Light Pos", (float*)&(PublicSingletonInstance(GLobalLight).m_BlockData.Position));
        PublicSingletonInstance(GLobalLight).updateBuffer();
        ImGui::Render();
        QtImGui::render();
    }
    void EditorRendererWidget::importMesh(const std::string filename)
    {
        auto pointPos = filename.find_last_of(".");
        auto lPos = filename.find_last_of("/");
        std::string meshName = filename.substr(lPos + 1, pointPos - 1 - lPos);
        auto testMesh = PublicSingletonInstance(Scene).CreateObject(meshName);
        testMesh.AddComponent<ModelComponent<AssimpNode>>(filename);
        testMesh.AddComponent<TransformComponent>();
    }
}