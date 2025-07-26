#!/bin/bash

rm -rf YoupiBanane
mkdir -p YoupiBanane/nop
mkdir -p YoupiBanane/Yeah

touch YoupiBanane/youpi.bad_extension
touch YoupiBanane/youpi.bla
echo -e '#!/bin/bash\n../ubuntu_cgi_tester' > YoupiBanane/youpi.bla

chmod +x YoupiBanane/youpi.bla

touch YoupiBanane/nop/youpi.bad_extension
touch YoupiBanane/nop/other.pouic

touch YoupiBanane/Yeah/not_happy.bad_extension
