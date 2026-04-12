# Repository Guidelines

## Project Structure & Module Organization

This is a Visual Studio 2022 DirectX 11 C++ solution in `Solo_Leveling.sln`.
The project is split into four modules: `Engine/` is the core DLL, `Client/` is the gameplay DLL, `GameApp/` is the runtime executable, and `Editor/` is the ImGui-based editor executable. Most modules keep headers in `Public/`, implementation files in `Private/`, and Visual Studio project files in `Default/`. Shared runtime content lives under `Resources/`, especially `Resources/ShaderFiles/`, `Resources/Textures/`, and `Resources/Models/`. Generated SDK headers/libs are copied to `EngineSDK/` and `ClientSDK/` by post-build batch files. Planning and progress notes are in `명세서/`.

## Build, Test, and Development Commands

- `msbuild Solo_Leveling.sln /p:Configuration=Debug /p:Platform=x64` builds the primary debug configuration.
- `msbuild Solo_Leveling.sln /p:Configuration=Release /p:Platform=x64` builds optimized binaries.
- `EngineSDK_Update.bat` refreshes `EngineSDK/` after Engine changes.
- `ClientSDK_Update.bat` refreshes `ClientSDK/` after Client changes.
- `RTTR_Copy.bat` copies RTTR runtime files when editor reflection dependencies change.

There is no automated test suite in this repository. Validate changes by building `Debug|x64` and running `GameApp` or `Editor` from their `Bin/` outputs.

## Coding Style & Naming Conventions

Follow the existing C++ style: tabs for indentation, `C`-prefixed class names such as `CGameObject`, `m_` member prefixes, PascalCase methods, and `NS_BEGIN(...)` / `NS_END` namespace macros. Initialization methods generally return `HRESULT`; use `SUCCEEDED` and `FAILED` checks consistently. Use `ENGINE_DLL` and `CLIENT_DLL` exports for externally used classes. Manage ownership through `Safe_AddRef`, `Safe_Release`, `Safe_Delete`, and `Safe_Delete_Array`; avoid raw `delete`.

Keep Engine independent of Editor-only libraries. Assimp, RTTR, ImGui, and ImGuizmo should remain under `Editor/` unless the architecture is intentionally changed.

## Testing Guidelines

For code changes, perform at least a `Debug|x64` solution build. For rendering, model loading, input, or editor panel changes, also run the affected executable and check the relevant scene or panel manually. When adding a new runtime asset, verify relative paths use the existing pattern, for example `../../Resources/ShaderFiles/...`.

## Codex Collaboration Notes

Do not directly modify C++ source/project files unless the user explicitly asks Codex to apply or implement the change. By default, inspect the code and provide diagnosis, design notes, and code blocks for the user to apply. Markdown planning files may be updated when the user explicitly asks for documentation changes.

Current planning sources of truth are `CLAUDE.md`, `명세서/Editor_전체_구현계획.md`, and daily notes under `명세서/`. As of 2026-04-12, Player/Body/Weapon work is in a prototype validation stage: `CPlayer` is a `CContainerObject`, `CBody_Player` and `CWeapon` are `CPartObject`s, Viewport picking must consider parts inside containers, and player animation selection should be designed as `PLAYER_ACTION + WEAPON_STATE -> AnimationName -> cached index` rather than hard-coded animation indices.

## Commit & Pull Request Guidelines

Recent history uses short Korean summary commits, for example `계획서 수정` and `애니메이션 전환 보간 처리 1차 구현`. Keep commits focused and describe the user-visible or architectural change. Pull requests should include a concise summary, build result, manual verification notes, linked issue or task when available, and screenshots or recordings for visible GameApp or Editor changes.
