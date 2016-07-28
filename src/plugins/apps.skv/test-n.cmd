echo OFF
for /l %%x in (11, 1, 20) do (
	echo start test instance %%x 
	mkdir test-%%x
	cd test-%%x	
	start ..\dsn.app.simple_kv.exe ..\config-n.ini
	cd ..
)
