$host.ui.RawUI.WindowTitle = "AI Developer Agent"

$venvPath = "D:\installs\venvs\aider"
$projectPath = "D:\Repos\2D_Game_1"

cd $projectPath
. "$venvPath\Scripts\Activate.ps1"

# Set your preferred models here
# --model: The high-level architect (Opus)
# --editor-model: The one that writes the files (Sonnet is excellent and cheaper for this)
aider `
  --model openrouter/anthropic/claude-opus-4.5 `
  --editor-model openrouter/anthropic/claude-3.5-sonnet `
  --read ai/.aider.instructions.md `
  --read ai/.aider.conventions.md `
  --read build.ps1 `
  --test-cmd "powershell -File build.ps1" `
  --cache-prompts `
  --yes