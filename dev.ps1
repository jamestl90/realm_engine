$host.ui.RawUI.WindowTitle = "AI Developer Agent"

$venvPath = "D:\installs\venvs\aider"
$projectPath = "D:\Repos\2D_Game_1"

cd $projectPath
. "$venvPath\Scripts\Activate.ps1"

aider `
  --model openrouter/anthropic/claude-opus-4.5 `
  --editor-model openrouter/anthropic/claude-3.5-sonnet `
  --read ai/.aider.instructions.md `
  --read ai/.aider.conventions.md `
  --read build.ps1 `
  --test-cmd "powershell -File build.ps1" `
  --cache-prompts `
  --no-auto-commits `
  --yes