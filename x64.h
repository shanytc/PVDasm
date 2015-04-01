// x64 Bit Declarations
#ifdef _M_X64
	#define GWL_USERDATA			GWLP_USERDATA
	#define GWL_WNDPROC				GWLP_WNDPROC
	#define DWL_MSGRESULT			DWLP_MSGRESULT
	#define DWL_MSGRESULT			DWLP_MSGRESULT
	#define GCL_HCURSOR				GCLP_HCURSOR
	#define IMAGE_OPTIONAL_HEADER	IMAGE_OPTIONAL_HEADER32 // force 32bit Optional header on 64Bit code.
	#define IMAGE_NT_HEADERS		IMAGE_NT_HEADERS32		// force 32bit NT header on 64Bit code.
	#define PIMAGE_THUNK_DATA		PIMAGE_THUNK_DATA32
	// x64 Functions
	//#define SetWindowLong	SetWindowLongPtr
	//#define SetClassLong	SetClassLongPtr

	
#endif