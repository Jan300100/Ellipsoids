#pragma once

// Link necessary d3d12 libraries.
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <cstdio>
#include <wtypes.h>

#include "Pipeline.h"
#include "Compute.h"

#include <iostream>

#include "Win32Window.h"
#include <time.h>

using namespace DirectX;


float CheckPixel(float x, float y, FXMMATRIX qtilde);


int CALLBACK wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
#ifdef _DEBUG
	//setup Console
	AllocConsole();
	FILE* pDummy;
	freopen_s(&pDummy, "CONIN$", "r", stdin);
	freopen_s(&pDummy, "CONOUT$", "w", stderr);
	freopen_s(&pDummy, "CONOUT$", "w", stdout);
#endif

	srand((unsigned int)time(nullptr));

	int w = 80, h = 60;

	//Win32Window window{ hInstance, 640, 480 };
	//Pipeline directx{&window };
	//Compute compute{ &directx, &window };

	//MSG msg = {};
	//while (msg.message != WM_QUIT)
	//{
	//	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	//	{
	//		::TranslateMessage(&msg);
	//		::DispatchMessage(&msg);
	//	}
	//	compute.Execute();
	//	directx.Render(compute.GetOutput());
	//}

	XMMATRIX Q_d
	{
		1,0,0,0,
		0,1,0,0,
		0,0,1,0,	
		0,0,0,-1
	}; //defines a sphere at the origin (xyzw-row * def *  xyzw-column to get resulting definition)

	XMMATRIX T_de{	XMMatrixLookAtLH( XMVectorSet(0,0,2,1),XMVectorSet(0,0,0,1), XMVectorSet(0,1,0,0)) }; //transforms camera to origin : transforms objects to camera space
	XMMATRIX T_ep{ XMMatrixPerspectiveFovLH(XM_PIDIV2, float(w) / h, 0.1f, 100.0f) };

	XMMATRIX T_dp{ T_de * T_ep };

	XMMATRIX T_pd{ XMMatrixInverse(nullptr, T_dp) };

	XMMATRIX Q_p{ T_pd * Q_d * XMMatrixTranspose(T_pd) };

	XMFLOAT4X4 Q_p_temp;
	XMStoreFloat4x4(&Q_p_temp, Q_p);
	

	//SHEAR == PER SPHERE

	// to create T_sp -> we need Q_p

	float T_sp_col2[4];
	for (size_t i = 0; i < 4; i++)
	{
		T_sp_col2[i] = -Q_p_temp(i, 2) / Q_p_temp(2, 2);
	}


	XMMATRIX T_sp
	{
		1,0,T_sp_col2[0],0,
		0,1,T_sp_col2[1],0,
		0,0,T_sp_col2[2],0,
		0,0,T_sp_col2[3],1
	}; //defines a sphere at the origin (xyzw-row * def *  xyzw-column to get resulting definition)

	XMMATRIX T_sd = T_sp * T_pd;

	// now we can create Q_s

	XMMATRIX Q_s{ (-1 / Q_p_temp(2, 2)) * (T_sp * Q_p * XMMatrixTranspose(T_sp))};
	XMFLOAT4X4 Q_s_temp; XMStoreFloat4x4(&Q_s_temp, Q_s);
	
	//now we need to find Q_tilde -> a simplified version of Q_s
	XMFLOAT3X3 Q_tilde_temp
	{
		Q_s_temp(0,0),Q_s_temp(0,1),Q_s_temp(0,3),
		Q_s_temp(1,0),Q_s_temp(1,1),Q_s_temp(1,3),
		Q_s_temp(3,0),Q_s_temp(3,1),Q_s_temp(3,3),
	};
	XMMATRIX Q_tilde = XMLoadFloat3x3(&Q_tilde_temp);

	//and now we can put in pixel values (x, y) and calculate z

	for (int i = 0; i < h; i++)
	{
		for (int j = 0; j < w; j++)
		{
			std::string output = (CheckPixel( - ((j / float(w)) - 0.5f) * 2.0f,-(( i / float(h)) - 0.5f)*2.0f, Q_tilde) > 0.0f) ? "X" : " ";
			std::cout << output;
		}
		std::cout << std::endl;
	}



	system("pause");
}

float CheckPixel(float x, float y, FXMMATRIX qtilde)
{
	XMFLOAT3X3 qTilde_temp;
	XMStoreFloat3x3(&qTilde_temp, qtilde);

	//rowVec * qtilde
	XMFLOAT3 r1
	{
		x * qTilde_temp(0,0) + y * qTilde_temp(1,0) + qTilde_temp(2,0),
		x * qTilde_temp(0,1) + y * qTilde_temp(1,1) + qTilde_temp(2,1),
		x * qTilde_temp(0,2) + y * qTilde_temp(1,2) + qTilde_temp(2,2)
	};

	//r1 * çolumnVec
	float zSquared = r1.x * x + r1.y * y + r1.z;
	return zSquared;

}
