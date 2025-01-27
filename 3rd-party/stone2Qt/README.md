stone2Qt
======================
Adaptation of STONE specifications and python scripts developed by Dropbox for generation of Qt-based projects. Currently includes code to generate Dropbox and various Google APIs. Used to generate googleQt and dropboxQt interfaces.

Disclaimer
==========
This is in-house project that osoftteam uses to generate part of Ardi codebase. osoftteam doesn't provide support for the project and doesn't guaranty it will be supported with new versions of Qt, Python or Dropbox STONE egg.

       
Requirement
============
1.pyhton3.5 - won't work with newer Python because of deprecated 'imp'
2.stone modules stone-0.1-py3.5.egg - should be in folder egg4STONE next to STONE
3.stone Dropbox API specification - inside this folder, see 'g-spec'

Usage
===========
To generate source from stone spec got to the spec folder and invoke command

python -m stone.cli <path-to-q4s.stoneg.py> . *.stone
or python3 if you are on Linux/OSX

Sample
=========
python -m stone.cli -a :all ../qgen/q4s.stoneg.py . *.stone

Google API generation
====================
There is script to perform STONE-generation for Google APIs

cd g-spec
./rollstone.csh

