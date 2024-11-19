Current status / things to fix
- DynParticles works sort of - the images it generates are messed up. I think it's a formula issue?
- I found a typo in dynrpg_dyntext.cpp where append_line was in place of an append_text and fixed that.
- DynParticles and RPGSS currently want to remove the ".png" from the filename, which is different from what DynRPG does.
- RPGSS doesn't load images but compiles just fine.
- A test project is included in /build/
