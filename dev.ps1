$host.ui.RawUI.WindowTitle = "AI Developer Agent"

$venvPath = "D:\installs\venvs\aider"
$projectPath = "D:\Repos\2D_Game_1"

cd $projectPath
. "$venvPath\Scripts\Activate.ps1"

aider `
  --model openrouter/anthropic/claude-3.5-sonnet `
  --editor-model openrouter/openai/gpt-4o-mini `
  --weak-model openrouter/openai/gpt-4o-mini `
  --read ai/.aider.instructions.md `
  --cache-prompts `
  --map-tokens 1024 `
  --no-auto-commits 