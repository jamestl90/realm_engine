$host.ui.RawUI.WindowTitle = "Coding Agent"

$venvPath = "D:\installs\venvs\aider"
$projectPath = "D:\Repos\2D_Game_1"

cd $projectPath
. "$venvPath\Scripts\Activate.ps1"

aider `
  --model openrouter/anthropic/claude-sonnet-4.5 `
  --read ai\.aider.coding.instructions `
  --edit-format diff `
  --map-tokens 2048 `
  --cache-prompts
