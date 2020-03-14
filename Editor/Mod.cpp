#include "stdafx.h"
#include "Mod.h"
#include "GlobalDef.h"
#include "Viewport.h"
#include "Node.h"
#include "Primative.h"
#include "Grid.h"
#include "DebugNew.h"

ToolKit::Editor::ModManager ToolKit::Editor::ModManager::m_instance;

ToolKit::Editor::ModManager::~ModManager()
{
	for (BaseMod* mod : m_modStack)
	{
		mod->UnInit();
		SafeDel(mod);
	}
}

ToolKit::Editor::ModManager* ToolKit::Editor::ModManager::GetInstance()
{
	return &m_instance;
}

void ToolKit::Editor::ModManager::Update(float deltaTime)
{
	if (m_modStack.empty())
	{
		return;
	}

	BaseMod* currentMod = m_modStack.back();
	currentMod->Update(deltaTime);
}

void ToolKit::Editor::ModManager::DispatchSignal(SignalId signal)
{
	if (m_modStack.empty())
	{
		return;
	}

	m_modStack.back()->Signal(signal);
}

void ToolKit::Editor::ModManager::SetMod(bool set, ModId mod)
{
	if (set)
	{
		if (m_modStack.back()->m_id != ModId::Base)
		{
			BaseMod* prevMod = m_modStack.back();
			SafeDel(prevMod);
			m_modStack.pop_back();
		}

		BaseMod* nextMod = nullptr;
		switch (mod)
		{
		case ToolKit::Editor::ModId::Select:
			nextMod = new SelectMod();
			break;
		case ToolKit::Editor::ModId::Cursor:
			nextMod = new CursorMod();
			break;
		case ToolKit::Editor::ModId::Move:
			break;
		case ToolKit::Editor::ModId::Rotate:
			break;
		case ToolKit::Editor::ModId::Scale:
			break;
		case ToolKit::Editor::ModId::Base:
		default:
			break;
		}

		assert(nextMod);
		if (nextMod != nullptr)
		{
			m_modStack.push_back(nextMod);
		}
	}
}

ToolKit::Editor::ModManager::ModManager()
{
	m_modStack.push_back(new BaseMod(ModId::Base));
}

ToolKit::Editor::BaseMod::BaseMod(ModId id)
{
	m_id = id;
	m_stateMachine = new StateMachine();
	m_terminate = false;
}

ToolKit::Editor::BaseMod::~BaseMod()
{
	SafeDel(m_stateMachine);
}

void ToolKit::Editor::BaseMod::Init()
{
}

void ToolKit::Editor::BaseMod::UnInit()
{
}

void ToolKit::Editor::BaseMod::Update(float deltaTime)
{
	m_stateMachine->Update(deltaTime);
}

void ToolKit::Editor::BaseMod::Signal(SignalId signal)
{
	m_stateMachine->Signal(signal);
}

ToolKit::Editor::StatePickingBase::StatePickingBase(std::string name)
	: State(name) 
{ 
	m_mouseData.resize(2); 
}

void ToolKit::Editor::StatePickingBase::TransitionIn(State* prevState)
{
}

void ToolKit::Editor::StatePickingBase::TransitionOut(State* nextState)
{
	if (StatePickingBase* baseState = dynamic_cast<StatePickingBase*> (nextState))
	{
		baseState->m_mouseData = m_mouseData;

		if (nextState->m_name != StateBeginPick().m_name)
		{
			baseState->m_pickData = m_pickData;
		}
	}

	m_pickData.clear();
}

void ToolKit::Editor::StateBeginPick::Update(float deltaTime)
{
}

std::string ToolKit::Editor::StateBeginPick::Signaled(SignalId signal)
{
	if (signal == LeftMouseBtnDownSgnl())
	{
		Viewport* vp = g_app->GetActiveViewport();
		if (vp != nullptr)
		{
			m_mouseData[0] = vp->GetLastMousePosScreenSpace();
		}
	}

	if (signal == LeftMouseBtnUpSgnl())
	{
		Viewport* vp = g_app->GetActiveViewport();
		if (vp != nullptr)
		{
			m_mouseData[0] = vp->GetLastMousePosScreenSpace();

			Ray ray = vp->RayFromMousePosition();
			Scene::PickData pd = g_app->m_scene.PickObject(ray, m_ignoreList);
			m_pickData.push_back(pd);

			if (g_app->m_pickingDebug)
			{
				if (pd.entity != nullptr)
				{
					g_app->m_hitMarker->m_node->m_translation = pd.pickPos;
				}
				DebugDrawPickingRay(ray);
			}

			return StateEndPick().m_name;
		}
	}

	if (signal == LeftMouseBtnDragSgnl())
	{
		return StateBeginBoxPick().m_name;
	}

	return std::string();
}

void ToolKit::Editor::StateBeginBoxPick::Update(float deltaTime)
{
}

std::string ToolKit::Editor::StateBeginBoxPick::Signaled(SignalId signal)
{
	if (signal == LeftMouseBtnUpSgnl())
	{
		// Frustum - AABB test.
		return StateEndPick().m_name;
	}

	if (signal == LeftMouseBtnDragSgnl())
	{
		Viewport* vp = g_app->GetActiveViewport();
		if (vp != nullptr)
		{
			m_mouseData[1] = vp->GetLastMousePosScreenSpace();

			auto drawSelectionRectangleFn = [this](ImDrawList* drawList) -> void
			{
				glm::vec2 min(FLT_MAX, FLT_MAX);
				glm::vec2 max(-FLT_MAX, -FLT_MAX);

				for (int i = 0; i < 2; i++)
				{
					min = glm::min(min, m_mouseData[i]);
					max = glm::max(max, m_mouseData[i]);
				}

				ImU32 col = ImColor(ImVec4(0.4f, 0.4f, 0.4f, 0.4f));
				drawList->AddRectFilled(GLM2IMVEC(min), GLM2IMVEC(max), col, 5.0f);
				
				col = ImColor(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
				drawList->AddRect(GLM2IMVEC(min), GLM2IMVEC(max), col, 5.0f, 15, 2.0f);
			};

			vp->m_drawCommands.push_back(drawSelectionRectangleFn);
		}
	}

	return std::string();
}

void ToolKit::Editor::StateEndPick::Update(float deltaTime)
{
}

std::string ToolKit::Editor::StateEndPick::Signaled(SignalId signal)
{
	// Keep picking.
	if (signal == LeftMouseBtnDownSgnl())
	{
		Viewport* vp = g_app->GetActiveViewport();
		if (vp != nullptr && vp->IsViewportQueriable())
		{
			m_mouseData[0] = vp->GetLastMousePosScreenSpace();
			return StateBeginPick().m_name;
		}
	}

	return std::string();
}

void ToolKit::Editor::SelectMod::Init()
{
	StateBeginPick* initialState = new StateBeginPick();
	m_stateMachine->m_currentState = initialState;
	initialState->m_ignoreList = { g_app->m_grid->m_id };

	m_stateMachine->PushState(initialState);
	m_stateMachine->PushState(new StateBeginBoxPick());
	m_stateMachine->PushState(new StateEndPick());
}

void ToolKit::Editor::SelectMod::Update(float deltaTime)
{
	BaseMod::Update(deltaTime);
	static bool stateTransition = false;

	if (m_stateMachine->m_currentState->m_name == StateBeginPick().m_name)
	{
		stateTransition = true;
	}

	if (m_stateMachine->m_currentState->m_name == StateEndPick().m_name && stateTransition)
	{
		stateTransition = false;

		StateEndPick* endPick = static_cast<StateEndPick*> (m_stateMachine->m_currentState);
		ApplySelection(endPick->m_pickData);
	}
}

void ToolKit::Editor::SelectMod::ApplySelection(std::vector<Scene::PickData>& pickedNtties)
{
	bool shiftClick = ImGui::GetIO().KeyShift;

	if (pickedNtties.size() == 1 && pickedNtties.back().entity == nullptr)
	{
		if (!shiftClick)
		{
			g_app->m_scene.ClearSelection();
		}
	}

	for (Scene::PickData& pd : pickedNtties)
	{
		Entity* e = pd.entity;		

		if (e && !shiftClick)
		{
			g_app->m_scene.ClearSelection();
			g_app->m_scene.AddToSelection(e->m_id);
		}

		if (e && shiftClick)
		{
			if (g_app->m_scene.IsSelected(e->m_id))
			{
				g_app->m_scene.RemoveFromSelection(e->m_id);
			}
			else
			{
				g_app->m_scene.AddToSelection(e->m_id);
			}
		}
	}
}

void ToolKit::Editor::CursorMod::Init()
{
	State* initialState = new StateBeginPick();
	m_stateMachine->m_currentState = initialState;

	m_stateMachine->PushState(initialState);
	m_stateMachine->PushState(new StateEndPick());
}

void ToolKit::Editor::CursorMod::Update(float deltaTime)
{
	BaseMod::Update(deltaTime);
	static bool stateTransition = false;

	if (m_stateMachine->m_currentState->m_name == StateBeginPick().m_name)
	{
		stateTransition = true;
	}

	if (m_stateMachine->m_currentState->m_name == StateEndPick().m_name && stateTransition)
	{
		stateTransition = false;

		StateEndPick* endPick = static_cast<StateEndPick*> (m_stateMachine->m_currentState);
		Scene::PickData& pd = endPick->m_pickData.back();
		g_app->m_cursor->m_pickPosition = pd.pickPos;
	}
}

void ToolKit::Editor::StatePickingBase::DebugDrawPickingRay(Ray ray)
{
	static std::shared_ptr<Arrow2d> rayMdl = nullptr;
	if (rayMdl == nullptr)
	{
		rayMdl = std::shared_ptr<Arrow2d>(new Arrow2d());
		g_app->m_scene.m_entitites.push_back(rayMdl.get());
	}

	rayMdl->m_node->m_translation = ray.position;
	rayMdl->m_node->m_orientation = ToolKit::RotationTo(ToolKit::X_AXIS, ray.direction);
}