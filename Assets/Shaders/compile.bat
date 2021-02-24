@echo on

for %%f in (%~dp0*.glsl) do (
	%~dp0glslc.exe %%f -o %~dp0%%~nf.spv
)
pause