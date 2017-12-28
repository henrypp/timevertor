@echo off

php "..\builder\make_resource.php" ".\src\resource.hpp"
php "..\builder\make_locale.php" TimeVertor timevertor ".\bin\i18n" ".\src\resource.hpp" ".\bin\timevertor.lng"

pause
