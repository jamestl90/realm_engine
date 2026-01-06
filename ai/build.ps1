$host.ui.RawUI.WindowTitle = "Build Agent"

$venvPath = "D:\installs\venvs\aider"
$projectPath = "D:\Repos\2D_Game_1"

cd $projectPath
. "$venvPath\Scripts\Activate.ps1"

aider `
  --model openrouter/meta-llama/llama-3-8b-instruct `
  --read ai\.aider.build.instructions `
  --lint-cmd "powershell -File ai\do_build.ps1" `
  --cache-prompts