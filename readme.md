This is a work-in-progress mod/hack that aims to allow modders to add new ships without having to replace existing ones, increasing the maximum amount of usable ships. This will only support the the latest windows version (v1.5.13) of FTL: Faster Than Light.
#Compiling/running
This project uses VC++2010. It contains inline assembler which will likely break if compiled using a different compiler.
In order to run this, compile it into a DLL, place the fonts image in the same directory as the DLL, then inject the DLL into FTLGame.exe using your favourite DLL injector.