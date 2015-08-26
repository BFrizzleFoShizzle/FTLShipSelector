#include "FTLShipSelector.h"
//black magic for getting a handle on our own DLL
EXTERN_C IMAGE_DOS_HEADER __ImageBase;

const float charW = 8.0f;

HANDLE FTLProcess;
DWORD glFinishPointer;
DWORD mouseClickPointer = 0x00400000+0x22BF3C;
GLuint texFont;
GLubyte texFontData[128][128][4];

char ourDirectory[MAX_PATH] = {0};

ShipDescriptor* selectedShip = NULL;
ShipDescriptor* firstShip = NULL;
int* selectedShipBase = NULL;

HWND ftlWindow = NULL;

char output[500];

//select next ship
void shipRotForward() {
	ShipDescriptor* currShip = selectedShip;
	//used for moving up the tree, the last ship we looped as
	ShipDescriptor* lastShip = selectedShip;
	while(selectedShip == currShip) {
		if(selectedShip->greaterThan != NULL && lastShip!=selectedShip->greaterThan) {
			selectedShip = selectedShip->greaterThan;
		} else {
			//have to move upwards
			if(selectedShip != firstShip) {
				//move to parent node, make sure we don't come down here again
				if(selectedShip->parent->greaterThan!=selectedShip) {
					lastShip = selectedShip;
					selectedShip = selectedShip->parent;
				} else {
					//have to loop again
					lastShip = selectedShip;
					selectedShip = selectedShip->parent;
					currShip=selectedShip;
				}
			} else {
				//we're the root, no parents, so reset to the least value node
				while(selectedShip->lessThan != NULL) {
					selectedShip = selectedShip->lessThan;
				}
				//we're back at the start, exit the loop
				break;
			}
		}
		//if we haven't come from the greater than node and it's not null, switch to it
		/*if(selectedShip->greaterThan != NULL && lastShip!=selectedShip->greaterThan) {
			selectedShip = selectedShip->greaterThan;
			break;
		} else {
			if(selectedShip == selectedShip->lessThan) {
				selectedShip = selectedShip->parent;
				break;
			}
			//Either have to go to parent.greaterThan or parent.parent
			if(selectedShip != firstShip) {
				//move up to the parent, and make sure we don't come down the same connection
				lastShip = selectedShip;
				selectedShip = selectedShip->parent;
				currShip = selectedShip->parent;
			} else {
				//We're the root, go back to the lowest child
				while(selectedShip->lessThan != NULL) {
					selectedShip = selectedShip->lessThan;
				}
				//we're back at the start, exit the loop
				break;
			}
		}*/
	}
	selectedShipBase = &(selectedShip->base);
}
/*
DWORD WINAPI mouseHookLoop (LPVOID lpParam) {
	while(true) {
		MSG msg;
		if (PeekMessage(&msg,0,0,0,PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Sleep(1);
	}
	return 0;
}
*/
void shipRotBackward() {
	ShipDescriptor* currShip = selectedShip;
	//used for moving up the tree, the last ship we looped as
	ShipDescriptor* lastShip = selectedShip;
	while(selectedShip == currShip) {
		//if we haven't come from the lesser than node and it's not null, switch to it
		if(selectedShip->lessThan != NULL && lastShip!=selectedShip->lessThan) {
			selectedShip = selectedShip->lessThan;
		} else {
			//have to move upwards
			if(selectedShip != firstShip) {
				//move up to the parent, and make sure we don't come down the same connection
				lastShip = selectedShip;
				selectedShip = selectedShip->parent;
				currShip = selectedShip->parent;
			} else {
				//We're the root, go back to the greatest child
				while(selectedShip->greaterThan != NULL) {
					selectedShip = selectedShip->greaterThan;
				}
				//we're back at the start, exit the loop
				break;
			}
		}
	}
	selectedShipBase = &(selectedShip->base);
}

//Don't inline or it'll mess up the hook's stack frame!
__declspec(noinline) void dealWithClick(void) {
	int x, y;
	__asm
	{
		//copy values to x and y
		push eax;
		mov eax, [ebp+0x14];
		mov x, eax;
		mov eax, [ebp+0x18];
		mov y, eax;
		pop eax
	}
	if(ftlWindow==NULL){
		ftlWindow = GetActiveWindow();
		sprintf(output,"FTLWindow %x",ftlWindow);
		MessageBox(NULL, output, "test", MB_OK + MB_ICONINFORMATION);
		GetWindowText(ftlWindow,output,50);
		MessageBox(NULL, output, "test", MB_OK + MB_ICONINFORMATION);
	}
	//check if in hangar
	if(*((bool*)0x0028EECC)) {
		//button 1
		if(x>25 && x<95 && y>175 && y<210) {
			shipRotBackward();
		}
		//button 2
		if(x>95 && x<165 && y>175 && y<210) {
			shipRotForward();
		}
	}
	char out[50];
	sprintf(out,"X: %i, Y: %i", x, y);
	//MessageBox(NULL, out, "test", MB_OK + MB_ICONINFORMATION);

}

void hookClick(void){
	__asm
	{
		//undo VC++ crap
		pop edi;
		pop ebx;
		//apply overwritten code
		movzx edx,word ptr [ebx+04];
		mov [esp], edx;
		//prep for func call:
		push eax;
		push ecx;
		push edx;
	}
	dealWithClick();
	__asm
	{
		//recover from function call:
		
		pop edx;
		pop ecx;
		pop eax;
		//do a push ret back to the code (8 bytes ahead of where we were
		push mouseClickPointer;
		push eax;
		mov eax, [esp+4];
		add eax, 7;
		mov [esp+4], eax;
		pop eax;
		ret;
	}
}

void drawStuff(void) {
	/*__asm{
		int 3;
	}*/
	ShipDescriptor* currShip = firstShip;
	float y = 60.0f;
	sprintf_s(output,"0 %x", selectedShipBase);
	drawString(200.0f,y,output);
	y+=20.0f;
	if(selectedShip!=NULL)
		sprintf_s(output,"1 %x %s", selectedShip, selectedShip->shipClass);
	drawString(200.0f,y,output);
	y+=20.0f;
	if(firstShip!=NULL)
		sprintf_s(output,"2 %x %s", firstShip, firstShip->shipClass);
	drawString(200.0f,y,output);
	//if in hangar, draw custom UI stuff
	if(*((bool*)0x0028EECC)) {
		//(5, 110) (185, 110) (185, 340) (5, 340)
		glColor3f(0.5,0.5,0.5);
		drawRect(5,110,180,231);
		glColor3f(0.3,0.3,0.3);
		drawRect(25,175,140,35);
		glColor4f(1,1,1,1);
		drawString(30,190,"Previous");
		drawString(115,190,"Next");
		//draw a cursor if we're covering it...
		if(ftlWindow!=NULL) {
			//3x3 square at cursor so the user knows where it is...
			glColor4f(1,1,1,1);
			POINT p;
			GetCursorPos(&p);
			ScreenToClient(ftlWindow,&p);
			sprintf(output,"Mousepos %i %i",p.x, p.y);
			drawString(200,190,output);
			if(p.x>5&&p.x<185&&p.y>110&&p.y<340){
				//draw cursor
				drawTriangle(p.x,p.y,p.x+10,p.y+10,p.x,p.y+14);
			}
		}
	}
};

void hookOpenGLFinish(void) {
	// EBP, EBX and EDI are pushed by VC++ automatically before this ASM block
	// code here is safe, just don't use any variables, cause they'll prolly screw up the registers
	// if you need variables, create a function and call it here, that will preserve the registers

	drawStuff();

	// reset registers and jump to opengl code
	__asm
	{
		pop edi;
		pop ebx;
		pop ebp;
		jmp glFinishPointer;
	}
};

DWORD WINAPI FTLM_Main (LPVOID lpParam)
{
	FTLProcess = GetCurrentProcess();
	//generate texture for font
	glGenTextures(1, &texFont);
	// Hook openGL finish
	glFinishPointer = (DWORD)GetProcAddress(GetModuleHandle("OPENGL32.dll"), "glFinish");
	RETHook6Byte((0x0025DBB8+0x00400000),hookOpenGLFinish,FTLProcess);
	//hook mouse click
	RETHook6Byte(mouseClickPointer,hookClick,FTLProcess);
	//font loading stuff
	//get our DLL's path, in a useful form
	WCHAR wcDllPath[MAX_PATH] = {0};
	GetModuleFileNameW((HINSTANCE)&__ImageBase, wcDllPath, _countof(wcDllPath));
	wcstombs(ourDirectory,wcDllPath,MAX_PATH);
	//strip DLL name
	*(strrchr(ourDirectory, '\\')+1) = '\x00';
	//work out file path to bitmap font
	char bitmapFile[MAX_PATH];
	FILE* fp;
	sprintf(bitmapFile, "%sfont.bmp", ourDirectory);
	fp = fopen(bitmapFile, "r");
	if(!fp) {
		MessageBox(NULL, "Missing fonts file! D: should be...", "Fail!", MB_OK + MB_ICONINFORMATION);
		MessageBox(NULL, bitmapFile, "Fail!", MB_OK + MB_ICONINFORMATION);
	}
	//load the bitmap font from our dir
	readBitmapFont(fp);

	// set up pointers to game stuff
	// wait for ships to be initialized
	while(true){
		ShipDescriptor* shipsDescExe;
		ReadProcessMemory(FTLProcess,(VOID*)(playerShipsExe),&shipsDescExe,4,NULL);
		// check if ships have been loaded
		char out[50];
		sprintf(out,"DescExe %x",shipsDescExe);
		MessageBox(NULL, out, "test", MB_OK + MB_ICONINFORMATION);
		Sleep(1000);
		if(shipsDescExe && shipsDescExe->base == 0x007923C8) {
			// descriptors are loaded, we can get the value and safely hook the functions now!
			firstShip = shipsDescExe;
			selectedShip = shipsDescExe;
			selectedShipBase = &(shipsDescExe->base);
			// overwrite instruction that gets player ship descriptor for ship creation
			// this means it gets our value instead of the game's value
			MovAddrReg6Byte(0x00400000+0x001D6B4A,"eax",&selectedShipBase);
			// NOP 1 byte (replaced instructions are 7 bytes, hook is 6)
			NOP(0x00400000+0x001D6B4A+6,1);
			break;
		}
	}
	/*
    CreateThread(NULL,0,&mouseHookLoop,NULL,0,NULL);
	HWND hwndConsole = FindWindow( NULL, "FTL" );
	HINSTANCE hInstance = (HINSTANCE)GetWindowLong(hwndConsole, GWL_HINSTANCE);
	//set mouse hook
	DWORD threadID = GetWindowThreadProcessId(hwndConsole,NULL);
	sprintf(output,"threadID %lu hInstance %lu",threadID, hInstance);
	MessageBox(NULL, output, "test", MB_OK + MB_ICONINFORMATION);
	//DWORD threadID = GetCurrentThreadId();
	SetWindowsHookEx(WH_MOUSE_LL,MouseHookProc,hInstance,NULL);
	while(true) {
		MSG msg;
		if (PeekMessage(&msg,0,0,0,PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Sleep(1);
	}
	*/
	while(!playerShip){
		ReadProcessMemory(FTLProcess,(VOID*)(0x400000+0x39BA90),&playerShip,4,NULL);
		Sleep(100);
	}
	return 0;
};

void drawString(float x, float y, char* text){
	// set up for text drawing
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glBindTexture(GL_TEXTURE_2D, texFont);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 128, 128, 0, GL_RGBA, GL_UNSIGNED_BYTE, texFontData);
	while(*text != 0) {
		drawChar(x, y, *text);
		x+=charW;
		text++;
	}
};

void drawTriangle (float x1,float y1,float x2,float y2,float x3,float y3) {
	glEnable(GL_COLOR_MATERIAL);
	glBegin(GL_TRIANGLES);
	 glVertex3f(x1, y1, 0.0);
	 glVertex3f(x2, y2, 0.0);
	 glVertex3f(x3, y3, 0.0);
	glEnd();
	glDisable(GL_COLOR_MATERIAL);
}

void drawRect (float x,float y,float w,float h) {
	glEnable(GL_COLOR_MATERIAL);
	glBegin(GL_QUADS);
	 glVertex3f(x, y, 0.0); 
	 glVertex3f(x+w, y, 0.0); 
	 glVertex3f(x+w, y+h, 0.0); 
	 glVertex3f(x, y+h, 0.0); 
	glEnd();
	glDisable(GL_COLOR_MATERIAL);
}

void drawChar(float x, float y, char text) {
	//For some reason v seems to be rounded down, so
	//we add an extra pixel to the v coord to fix this
	float pxl = 1.0/128;
	// width/height of a char in texture space
	float charSize = 1.0f/16;
	// t (y) texture coord of char
	float charT = ((float)(text%16))*charSize;
	// s (x) texture coord of char
	float charS = ((float)(text/16))*charSize;
	
	glEnable(GL_TEXTURE_2D);
	glBegin(GL_QUADS);
	 glTexCoord2f(charS, charT+pxl);
	 glVertex3f(x, y, 0.0); 
	 glTexCoord2f(charS+charSize, charT+pxl);
	 glVertex3f(x+charW, y, 0.0); 
	 glTexCoord2f(charS+charSize, charT+charSize+pxl);
	 glVertex3f(x+charW, y+charW, 0.0); 
	 glTexCoord2f(charS, charT+charSize+pxl);
	 glVertex3f(x, y+charW, 0.0); 
	glEnd();
	glDisable(GL_TEXTURE_2D);
};

void readBitmapFont(FILE* fp) {
	//read up to the image data offset
	for(int i=0;i<0xA;i++) {
		fgetc(fp);
	}
	int dataOffset = 0;
	fread(&dataOffset, 4, 1, fp);
	//discard everything but the raw image data
	for(int i=0;i<dataOffset-0xE;i++) {
		fgetc(fp);
	}
	//read in image
	//(128*128)/8 = amount of raw data
	for(int i=0;i<(128*128)/8;i++) {
		int currByte = fgetc(fp);
		for(int currBit=0;currBit<8;currBit++) {
			int x = ((i*8)+currBit)%128;
			//fix
			int y = 128-(((i*8)+currBit)/128);
			//I'm not american, shocking!
			int colour = ((currByte>>(7-currBit))&1)*255;
			texFontData[y][x][0] = colour;
			texFontData[y][x][1] = colour;
			texFontData[y][x][2] = colour;
			texFontData[y][x][3] = colour;
		}
	}
};

BOOL WINAPI DllMain (HINSTANCE hModule, DWORD dwAttached, LPVOID lpvReserved)
{
	// DLL entry point
	// make us a thread!
    if (dwAttached == DLL_PROCESS_ATTACH) {
        CreateThread(NULL,0,&FTLM_Main,NULL,0,NULL);
    }
    return 1;
};