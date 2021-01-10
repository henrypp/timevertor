@echo off

php "..\builder\make_resource.php" ".\src\resource.h"
php "..\builder\make_locale.php" "TimeVertor" "timevertor" ".\bin\i18n" ".\src\resource.h" ".\src\resource.rc" ".\bin\timevertor.lng"
copy /y ".\bin\timevertor.lng" ".\bin\32\timevertor.lng"
copy /y ".\bin\timevertor.lng" ".\bin\64\timevertor.lng"

pause
