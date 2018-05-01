rmdir /S /Q tjmod_old
rename tjmod tjmod_old
mkdir tjmod

copy ..\..\src\release\cgame_mp_x86.dll tjmod\cgame_mp_x86.dll
copy ..\..\src\release\qagame_mp_x86.dll tjmod\qagame_mp_x86.dll
copy ..\..\src\release\ui_mp_x86.dll tjmod\ui_mp_x86.dll

xcopy ..\..\tjmod tjmod /E /I

rename tjmod.pk3 tjmod_old.pk3
del tjmod.pk3

7za a -tzip -r tjmod.pk3 ./tjmod/*