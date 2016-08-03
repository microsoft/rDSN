@echo OFF
:loop
echo port = %port% >> run.txt
ping localhost  >> run.txt
goto loop
