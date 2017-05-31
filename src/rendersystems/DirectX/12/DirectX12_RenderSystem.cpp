/// @file
/// @version 4.3
/// 
/// @section LICENSE
/// 
/// This program is free software; you can redistribute it and/or modify it under
/// the terms of the BSD license: http://opensource.org/licenses/BSD-3-Clause

#ifdef _DIRECTX12
#include <d3d12.h>
#include <dxgi1_4.h>
#include <stdio.h>

#include <gtypes/Rectangle.h>
#include <gtypes/Vector2.h>
#include <hltypes/hexception.h>
#include <hltypes/hlog.h>
#include <hltypes/hplatform.h>
#include <hltypes/hresource.h>
#include <hltypes/hthread.h>

#include "april.h"
#include "DirectX12_PixelShader.h"
#include "DirectX12_RenderSystem.h"
#include "DirectX12_Texture.h"
#include "DirectX12_VertexShader.h"
#include "Image.h"
#include "Keys.h"
#include "Platform.h"
#include "RenderState.h"
#include "Timer.h"
#include "WinUWP.h"
#include "WinUWP_Window.h"

#define SHADER_PATH "april/"
#define VERTEX_BUFFER_COUNT 65536

#define __EXPAND(x) x

#define LOAD_SHADER(name, type, file) \
	if (name == NULL) \
	{ \
		name = (DirectX12_ ## type ## Shader*)this->create ## type ## ShaderFromResource(SHADER_PATH #type "Shader_" #file ".cso"); \
	}

#define _SELECT_SHADER(useTexture, useColor, type) \
	(useTexture ? (useColor ? this->shaderColoredTextured ## type : this->shaderTextured ## type) : (useColor ? this->shaderColored ## type : this->shader ## type));

using namespace Microsoft::WRL;
using namespace Windows::Graphics::Display;

namespace april
{
	static inline void _TRY_UNSAFE(HRESULT hr, chstr errorMessage)
	{
		if (FAILED(hr))
		{
			throw Exception(hsprintf("%s - HRESULT: 0x%08X", errorMessage.cStr(), hr));
		}
	}

	static ColoredTexturedVertex static_ctv[VERTEX_BUFFER_COUNT];

	D3D_PRIMITIVE_TOPOLOGY DirectX12_RenderSystem::_dx12RenderOperations[] =
	{
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST,	// ROP_TRIANGLE_LIST
		D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP,	// ROP_TRIANGLE_STRIP
		D3D_PRIMITIVE_TOPOLOGY_LINELIST,		// ROP_LINE_LIST
		D3D_PRIMITIVE_TOPOLOGY_LINESTRIP,		// ROP_LINE_STRIP
		D3D_PRIMITIVE_TOPOLOGY_POINTLIST,		// ROP_POINT_LIST
		D3D_PRIMITIVE_TOPOLOGY_UNDEFINED,		// triangle fans are deprecated in DX12
	};

	DirectX12_RenderSystem::DirectX12_RenderSystem() : DirectX_RenderSystem(), deviceState_constantBufferChanged(true)
	{
		this->name = april::RenderSystemType::DirectX12.getName();
		this->vertexBufferIndex = 0;
	}

	DirectX12_RenderSystem::~DirectX12_RenderSystem()
	{
		this->destroy();
	}

	void DirectX12_RenderSystem::_deviceInit()
	{
		this->d3dDevice = nullptr;
		this->swapChain = nullptr;
		/*
		this->samplerLinearWrap = nullptr;
		this->samplerLinearClamp = nullptr;
		this->samplerNearestWrap = nullptr;
		this->samplerNearestClamp = nullptr;
		*/
		for_iter (i, 0, MAX_VERTEX_BUFFERS)
		{
			this->vertexBuffers[MAX_VERTEX_BUFFERS] = nullptr;
		}
		this->constantBuffer = nullptr;
		this->vertexShaderPlain = NULL;
		this->vertexShaderTextured = NULL;
		this->vertexShaderColored = NULL;
		this->vertexShaderColoredTextured = NULL;
		this->pixelShaderMultiply = NULL;
		this->pixelShaderAlphaMap = NULL;
		this->pixelShaderLerp = NULL;
		this->pixelShaderTexturedMultiply = NULL;
		this->pixelShaderTexturedAlphaMap = NULL;
		this->pixelShaderTexturedLerp = NULL;
		this->deviceState_constantBufferChanged = true;
	}

	bool DirectX12_RenderSystem::_deviceCreate(Options options)
	{
		this->setViewport(grect(0.0f, 0.0f, april::getSystemInfo().displayResolution));
		return true;
	}

	bool DirectX12_RenderSystem::_deviceDestroy()
	{
		_HL_TRY_DELETE(this->vertexShaderPlain);
		_HL_TRY_DELETE(this->vertexShaderTextured);
		_HL_TRY_DELETE(this->vertexShaderColored);
		_HL_TRY_DELETE(this->vertexShaderColoredTextured);
		_HL_TRY_DELETE(this->pixelShaderMultiply);
		_HL_TRY_DELETE(this->pixelShaderAlphaMap);
		_HL_TRY_DELETE(this->pixelShaderLerp);
		_HL_TRY_DELETE(this->pixelShaderTexturedMultiply);
		_HL_TRY_DELETE(this->pixelShaderTexturedAlphaMap);
		_HL_TRY_DELETE(this->pixelShaderTexturedLerp);
		this->setViewport(grect(0.0f, 0.0f, april::getSystemInfo().displayResolution));
		return true;
	}

	void DirectX12_RenderSystem::_deviceAssignWindow(Window* window)
	{
		unsigned int dxgiFactoryFlags = 0;
#ifndef _DEBUG
		if (this->options.debugInfo)
#endif
		{
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
			}
			dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		}
		_TRY_UNSAFE(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&this->dxgiFactory)), "Unable to create DXGI factor!");
		// get DX12-capable hardware adapter
		ComPtr<IDXGIAdapter1> adapter = nullptr;
		DXGI_ADAPTER_DESC1 adapterDesc;
		UINT adapterIndex = 0;
		while (this->dxgiFactory->EnumAdapters1(adapterIndex, &adapter) != DXGI_ERROR_NOT_FOUND)
		{
			adapter->GetDesc1(&adapterDesc);
			if ((adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 && SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
			{
				break;
			}
			++adapterIndex;
		}
		HRESULT hr;
		if (adapter != nullptr)
		{
			hr = D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&this->d3dDevice));
			if (FAILED(hr))
			{
				hlog::write(logTag, "Hardware device not available. Falling back to WARP device.");
				adapter = nullptr;
			}
		}
		else
		{
			hlog::write(logTag, "Unable to find hardware adapter. Falling back to WARP device.");
		}
		// no valid adapter, use WARP
		if (adapter == nullptr)
		{
			ComPtr<IDXGIAdapter> warpAdapter = nullptr;
			_TRY_UNSAFE(this->dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)), "Unable to create DX12 device! WARP device not available!");
			hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&this->d3dDevice));
		}
		_TRY_UNSAFE(hr, "Unable to create DX12 device!");
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		_TRY_UNSAFE(this->d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&this->commandQueue)), "Unable to create command queue!");
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FRAME_COUNT;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		_TRY_UNSAFE(this->d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&this->rtvHeap)), "Unable to create RTV heap!");
		this->rtvDescSize = this->d3dDevice->GetDescriptorHandleIncrementSize(rtvHeapDesc.Type);
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 1;
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		_TRY_UNSAFE(this->d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&this->srvHeap)), "Unable to create SRV heap!");
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		_TRY_UNSAFE(this->d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&this->dsvHeap)), "Unable to create DSV heap!");
		for_iter (i, 0, FRAME_COUNT)
		{
			_TRY_UNSAFE(this->d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&this->commandAllocators[i])), hsprintf("Unable to create command allocator %d!", i));
		}
		// create synchronization objects
		_TRY_UNSAFE(this->d3dDevice->CreateFence(this->fenceValues[this->currentFrame], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&this->fence)), "Unable to create fence");
		++this->fenceValues[this->currentFrame];
		this->fenceEvent = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		// configure device
		this->updateWindowSize(false);
		this->_configureDevice();
		// default shaders
		LOAD_SHADER(this->vertexShaderPlain, Vertex, Plain);
		LOAD_SHADER(this->vertexShaderTextured, Vertex, Textured);
		LOAD_SHADER(this->vertexShaderColored, Vertex, Colored);
		LOAD_SHADER(this->vertexShaderColoredTextured, Vertex, ColoredTextured);
		LOAD_SHADER(this->pixelShaderMultiply, Pixel, Multiply);
		LOAD_SHADER(this->pixelShaderAlphaMap, Pixel, AlphaMap);
		LOAD_SHADER(this->pixelShaderLerp, Pixel, Lerp);
		LOAD_SHADER(this->pixelShaderTexturedMultiply, Pixel, TexturedMultiply);
		LOAD_SHADER(this->pixelShaderTexturedAlphaMap, Pixel, TexturedAlphaMap);
		LOAD_SHADER(this->pixelShaderTexturedLerp, Pixel, TexturedLerp);
		// input layouts for default shaders
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutDescPlain[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutDescColored[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutDescTextured[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		static const D3D12_INPUT_ELEMENT_DESC inputLayoutDescColoredTextured[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
		D3D12_RENDER_TARGET_BLEND_DESC renderTargetAlpha;
		renderTargetAlpha.BlendEnable = true;
		renderTargetAlpha.LogicOpEnable = false;
		renderTargetAlpha.RenderTargetWriteMask = (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE);
		renderTargetAlpha.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		renderTargetAlpha.SrcBlendAlpha = D3D12_BLEND_ONE;
		renderTargetAlpha.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		renderTargetAlpha.BlendOp = D3D12_BLEND_OP_ADD;
		renderTargetAlpha.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		renderTargetAlpha.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		D3D12_RENDER_TARGET_BLEND_DESC renderTargetAdd;
		renderTargetAdd.BlendEnable = true;
		renderTargetAdd.LogicOpEnable = false;
		renderTargetAdd.RenderTargetWriteMask = (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE);
		renderTargetAdd.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		renderTargetAdd.SrcBlendAlpha = D3D12_BLEND_ONE;
		renderTargetAdd.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		renderTargetAdd.BlendOp = D3D12_BLEND_OP_ADD;
		renderTargetAdd.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		renderTargetAdd.DestBlend = D3D12_BLEND_ONE;
		D3D12_RENDER_TARGET_BLEND_DESC renderTargetSubtract;
		renderTargetSubtract.BlendEnable = true;
		renderTargetSubtract.LogicOpEnable = false;
		renderTargetSubtract.RenderTargetWriteMask = (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE);
		renderTargetSubtract.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		renderTargetSubtract.SrcBlendAlpha = D3D12_BLEND_ONE;
		renderTargetSubtract.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		renderTargetSubtract.BlendOp = D3D12_BLEND_OP_REV_SUBTRACT;
		renderTargetSubtract.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		renderTargetSubtract.DestBlend = D3D12_BLEND_ONE;
		D3D12_RENDER_TARGET_BLEND_DESC renderTargetOverwrite;
		renderTargetOverwrite.BlendEnable = true;
		renderTargetOverwrite.LogicOpEnable = false;
		renderTargetOverwrite.RenderTargetWriteMask = (D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE);
		renderTargetOverwrite.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		renderTargetOverwrite.SrcBlendAlpha = D3D12_BLEND_ONE;
		renderTargetOverwrite.DestBlendAlpha = D3D12_BLEND_ZERO;
		renderTargetOverwrite.BlendOp = D3D12_BLEND_OP_ADD;
		renderTargetOverwrite.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		renderTargetOverwrite.DestBlend = D3D12_BLEND_ZERO;
		const D3D12_DEPTH_STENCILOP_DESC defaultStencilOperation = { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
		// indexed data
		this->inputLayoutDescs.clear();
		this->inputLayoutDescs += { inputLayoutDescPlain, _countof(inputLayoutDescPlain) };
		this->inputLayoutDescs += { inputLayoutDescColored, _countof(inputLayoutDescColored) };
		this->inputLayoutDescs += { inputLayoutDescTextured, _countof(inputLayoutDescTextured) };
		this->inputLayoutDescs += { inputLayoutDescColoredTextured, _countof(inputLayoutDescColoredTextured) };
		this->vertexShaders.clear();
		this->vertexShaders += this->vertexShaderPlain;
		this->vertexShaders += this->vertexShaderColored;
		this->vertexShaders += this->vertexShaderTextured;
		this->vertexShaders += this->vertexShaderColoredTextured;
		this->pixelShaders.clear();
		this->pixelShaders += this->pixelShaderMultiply;
		this->pixelShaders += this->pixelShaderAlphaMap;
		this->pixelShaders += this->pixelShaderLerp;
		this->pixelShaders += this->pixelShaderTexturedMultiply;
		this->pixelShaders += this->pixelShaderTexturedAlphaMap;
		this->pixelShaders += this->pixelShaderTexturedLerp;
		this->blendStateRenderTargets.clear();
		this->blendStateRenderTargets += renderTargetAlpha;
		this->blendStateRenderTargets += renderTargetAdd;
		this->blendStateRenderTargets += renderTargetSubtract;
		this->blendStateRenderTargets += renderTargetOverwrite;
		this->primitiveTopologyTypes.clear();
		this->primitiveTopologyTypes += D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		this->primitiveTopologyTypes += D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
		this->primitiveTopologyTypes += D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		// pipeline states
		D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
		state.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		state.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		state.RasterizerState.FrontCounterClockwise = false;
		state.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		state.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		state.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		state.RasterizerState.DepthClipEnable = true;
		state.RasterizerState.MultisampleEnable = false;
		state.RasterizerState.AntialiasedLineEnable = false;
		state.RasterizerState.ForcedSampleCount = 0;
		state.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
		state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
		for_iter (i, 1, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
		{
			state.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
		}
		state.DSVFormat = DXGI_FORMAT_D16_UNORM;
		state.SampleMask = UINT_MAX;
		state.NumRenderTargets = 1;
		state.SampleDesc.Count = 1;
		state.SampleDesc.Quality = 0;
		state.BlendState.AlphaToCoverageEnable = false;
		state.BlendState.IndependentBlendEnable = false;
		state.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		state.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		state.DepthStencilState.StencilEnable = false;
		state.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
		state.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
		state.DepthStencilState.FrontFace = defaultStencilOperation;
		state.DepthStencilState.BackFace = defaultStencilOperation;
		// dynamic properties
		int pixelIndex = -1;
		for_iter (i, 0, this->inputLayoutDescs.size())
		{
			state.InputLayout = this->inputLayoutDescs[i];
			state.VS.pShaderBytecode = (unsigned char*)this->vertexShaders[i]->shaderData;
			state.VS.BytecodeLength = (SIZE_T)this->vertexShaders[i]->shaderData.size();
			for_iter (j, 0, pixelShaders.size() / 2)
			{
				pixelIndex = j + (i / (this->inputLayoutDescs.size() / 2)) * (this->pixelShaders.size() / 2);
				state.pRootSignature = this->rootSignatures[pixelIndex / (pixelShaders.size() / 2)].Get();
				state.PS.pShaderBytecode = (unsigned char*)this->pixelShaders[pixelIndex]->shaderData;
				state.PS.BytecodeLength = (SIZE_T)this->pixelShaders[pixelIndex]->shaderData.size();
				for_iter (k, 0, this->blendStateRenderTargets.size())
				{
					for_iter (m, 0, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT)
					{
						state.BlendState.RenderTarget[m] = this->blendStateRenderTargets[k];
					}
					for_iter (m, 0, this->primitiveTopologyTypes.size())
					{
						state.PrimitiveTopologyType = this->primitiveTopologyTypes[m];
						state.DepthStencilState.DepthEnable = false;
						_TRY_UNSAFE(this->d3dDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&this->pipelineStates[i][j][k][m][0])), "Unable to create graphics pipeline state!");
						state.DepthStencilState.DepthEnable = true;
						_TRY_UNSAFE(this->d3dDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&this->pipelineStates[i][j][k][m][1])), "Unable to create graphics pipeline state!");
					}
				}
			}
		}
		this->deviceState_pipelineState = this->pipelineStates[0][0][0][0][0];
		this->deviceState_rootSignature = this->rootSignatures[0];
		_TRY_UNSAFE(this->d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, this->commandAllocators[this->currentFrame].Get(),
			this->deviceState_pipelineState.Get(), IID_PPV_ARGS(&this->commandList)), "Unable to create command allocators state!");
		this->uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		this->uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		this->uploadHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		this->uploadHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		this->uploadHeapProperties.CreationNodeMask = 1;
		this->uploadHeapProperties.VisibleNodeMask = 1;
		//this->vertexBufferData.pSysMem = NULL;
		//this->vertexBufferData.SysMemPitch = 0;
		//this->vertexBufferData.SysMemSlicePitch = 0;
		this->vertexBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		this->vertexBufferDesc.Alignment = 0;
		this->vertexBufferDesc.Width = 10000;
		this->vertexBufferDesc.Height = 1;
		this->vertexBufferDesc.DepthOrArraySize = 1;
		this->vertexBufferDesc.MipLevels = 1;
		this->vertexBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		this->vertexBufferDesc.SampleDesc.Count = 1;
		this->vertexBufferDesc.SampleDesc.Quality = 0;
		this->vertexBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		this->vertexBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;


		CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

		hr = d3dDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&this->vertexBuffers[0]));
		hr = d3dDevice->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
			D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&this->vertexBuffers[1]));
		if (FAILED(hr))
		{
			throw Exception("Unable to create vertex buffer!");
		}


		//ComPtr<ID3D12Resource> vertexBufferUpload;
		hr = this->d3dDevice->CreateCommittedResource(&this->uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &this->vertexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&this->vertexBufferUploads[0]));
		hr = this->d3dDevice->CreateCommittedResource(&this->uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &this->vertexBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&this->vertexBufferUploads[1]));
		if (FAILED(hr))
		{
			throw Exception("Unable to create vertex buffer upload!");
		}
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = FRAME_COUNT;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		_TRY_UNSAFE(this->d3dDevice->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&this->cbvHeap)), "Unable to create constant buffer view!");
		this->constantBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		this->constantBufferDesc.Alignment = 0;
		this->constantBufferDesc.Width = FRAME_COUNT * ALIGNED_CONSTANT_BUFFER_SIZE;
		this->constantBufferDesc.Height = 1;
		this->constantBufferDesc.DepthOrArraySize = 1;
		this->constantBufferDesc.MipLevels = 1;
		this->constantBufferDesc.Format = DXGI_FORMAT_UNKNOWN;
		this->constantBufferDesc.SampleDesc.Count = 1;
		this->constantBufferDesc.SampleDesc.Quality = 0;
		this->constantBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		this->constantBufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;



		_TRY_UNSAFE(this->d3dDevice->CreateCommittedResource(&this->uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &this->constantBufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&this->constantBuffer)), "Unable to create constant buffer!");

		// Create constant buffer views to access the upload buffer.
		D3D12_GPU_VIRTUAL_ADDRESS cbvGpuAddress = this->constantBuffer->GetGPUVirtualAddress();
		D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = this->cbvHeap->GetCPUDescriptorHandleForHeapStart();
		this->cbvDescSize = this->d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
		desc.SizeInBytes = ALIGNED_CONSTANT_BUFFER_SIZE;
		for_iter (i, 0, FRAME_COUNT)
		{
			desc.BufferLocation = cbvGpuAddress;
			d3dDevice->CreateConstantBufferView(&desc, cbvCpuHandle);
			cbvGpuAddress += desc.SizeInBytes;
			cbvCpuHandle.ptr += this->cbvDescSize;
		}

		D3D12_RANGE readRange = {};
		readRange.Begin = 0;
		readRange.End = 0;
		this->constantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&this->mappedConstantBuffer));


		_TRY_UNSAFE(this->commandList->Close(), "Unable to close command list!");
		ID3D12CommandList* ppCommandLists[] = { this->commandList.Get() };
		this->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
		this->_waitForGpu();
		// initial calls
		_TRY_UNSAFE(this->commandAllocators[this->currentFrame]->Reset(), hsprintf("Unable to reset command allocator %d!", this->currentFrame));
		_TRY_UNSAFE(this->commandList->Reset(this->commandAllocators[this->currentFrame].Get(), this->deviceState_pipelineState.Get()), "Unable to reset command list!");
		PIXBeginEvent(this->commandList.Get(), 0, L"");
		this->_deviceClear(true);
		this->presentFrame();
		this->setOrthoProjection(gvec2((float)window->getWidth(), (float)window->getHeight()));
	}

	void DirectX12_RenderSystem::_deviceReset()
	{
		DirectX_RenderSystem::_deviceReset();
		// possible Microsoft bug, required for SwapChainPanel to update its layout 
		//reinterpret_cast<IUnknown*>(WinUWP::App->Overlay)->QueryInterface(IID_PPV_ARGS(&this->swapChainNative));
		//this->swapChainNative->SetSwapChain(this->swapChain.Get());
	}

	void DirectX12_RenderSystem::_deviceSetupCaps()
	{
		// depends on FEATURE_LEVEL, while 9.3 supports 4096, 9.2 and 9.1 support only 2048 so using 2048 is considered safe
		this->caps.maxTextureSize = D3D_FL9_1_REQ_TEXTURE1D_U_DIMENSION;
		this->caps.npotTexturesLimited = true;
		this->caps.npotTextures = false; // because of usage of feature level 9_3
	}

	void DirectX12_RenderSystem::_deviceSetup()
	{
		// not used
	}

	// Wait for pending GPU work to complete.
	void DirectX12_RenderSystem::_waitForGpu()
	{
		HRESULT hr = this->commandQueue->Signal(this->fence.Get(), this->fenceValues[this->currentFrame]);
		if (FAILED(hr))
		{
			throw Exception("Could not Signal command queue!");
		}
		hr = this->fence->SetEventOnCompletion(this->fenceValues[this->currentFrame], this->fenceEvent);
		if (FAILED(hr))
		{
			throw Exception("Could not Signal command queue!");
		}
		WaitForSingleObjectEx(this->fenceEvent, INFINITE, FALSE);
		++this->fenceValues[this->currentFrame];
	}

	void DirectX12_RenderSystem::_configureDevice()
	{
		this->_waitForGpu();
		for_iter (i, 0, FRAME_COUNT)
		{
			this->renderTargets[i] = nullptr;
			this->fenceValues[i] = this->fenceValues[this->currentFrame];
		}
		// swap chain
		float dpiRatio = WinUWP::getDpiRatio(this->dpi);
		this->outputSize.Width = (float)hmax(hround(this->logicalSize.Width * dpiRatio), 1);
		this->outputSize.Height = (float)hmax(hround(this->logicalSize.Width * dpiRatio), 1);
		DXGI_MODE_ROTATION displayRotation = this->_getDxgiRotation();
		if (displayRotation != DXGI_MODE_ROTATION_ROTATE90 && displayRotation != DXGI_MODE_ROTATION_ROTATE270)
		{
			this->d3dRenderTargetSize.Width = this->outputSize.Width;
			this->d3dRenderTargetSize.Height = this->outputSize.Height;
		}
		else
		{
			this->d3dRenderTargetSize.Width = this->outputSize.Height;
			this->d3dRenderTargetSize.Height = this->outputSize.Width;
		}
		if (this->swapChain != nullptr)
		{
			this->_resizeSwapChain(april::window->getWidth(), april::window->getHeight());
		}
		else
		{
			this->_createSwapChain(april::window->getWidth(), april::window->getHeight());
		}
		// other
		CD3DX12_DESCRIPTOR_RANGE ranges[3];
		ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		CD3DX12_ROOT_PARAMETER parameters[3];
		parameters[0].InitAsDescriptorTable(1, ranges, D3D12_SHADER_VISIBILITY_VERTEX);
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(1, parameters, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
		ComPtr<ID3DBlob> pSignature;
		ComPtr<ID3DBlob> pError;
		_TRY_UNSAFE(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf()), "Unable to serialize root signature!");
		_TRY_UNSAFE(this->d3dDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&this->rootSignatures[0])), "Unable to create root signature!");
		// other
		ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
		parameters[1].InitAsDescriptorTable(1, &ranges[1], D3D12_SHADER_VISIBILITY_PIXEL);
		CD3DX12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 0, 0,
			D3D12_COMPARISON_FUNC_NEVER, D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK, 0.0f, D3D12_FLOAT32_MAX, D3D12_SHADER_VISIBILITY_PIXEL);
		rootSignatureDesc.Init(2, parameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
			D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS);
		_TRY_UNSAFE(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignature.GetAddressOf(), pError.GetAddressOf()), "Unable to serialize root signature!");
		_TRY_UNSAFE(this->d3dDevice->CreateRootSignature(0, pSignature->GetBufferPointer(), pSignature->GetBufferSize(), IID_PPV_ARGS(&this->rootSignatures[1])), "Unable to create root signature!");
		// TODOuwp - create more than one sampler
		/*
		// texture samplers
		D3D12_SAMPLER_DESC samplerDesc;
		memset(&samplerDesc, 0, sizeof(samplerDesc));
		samplerDesc.MaxAnisotropy = 0;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		samplerDesc.ComparisonFunc = D3D12_COMPARISON_NEVER;
		samplerDesc.BorderColor[0] = 0.0f;
		samplerDesc.BorderColor[1] = 0.0f;
		samplerDesc.BorderColor[2] = 0.0f;
		samplerDesc.BorderColor[3] = 0.0f;
		// linear + wrap
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
		this->d3dDevice->CreateSamplerState(&samplerDesc, &this->samplerLinearWrap);
		// linear + clamp
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_CLAMP;
		this->d3dDevice->CreateSamplerState(&samplerDesc, &this->samplerLinearClamp);
		// nearest neighbor + wrap
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_WRAP;
		this->d3dDevice->CreateSamplerState(&samplerDesc, &this->samplerNearestWrap);
		// nearest neighbor + clamp
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_CLAMP;
		this->d3dDevice->CreateSamplerState(&samplerDesc, &this->samplerNearestClamp);
		*/
		// other
		//this->_deviceClear(true);
		//this->presentFrame();
	}

	void DirectX12_RenderSystem::_createSwapChain(int width, int height)
	{
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = width;
		swapChainDesc.Height = height;
		swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.Stereo = false;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = FRAME_COUNT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // UWP apps MUST use _FLIP_
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
		swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		ComPtr<IDXGISwapChain1> swapChain;
		_TRY_UNSAFE(this->dxgiFactory->CreateSwapChainForCoreWindow(this->commandQueue.Get(), reinterpret_cast<IUnknown*>(this->coreWindow.Get()),
			&swapChainDesc, nullptr, &swapChain), "Unable to create swap chain!");
		_TRY_UNSAFE(swapChain.As(&this->swapChain), "Unable to cast swap chain to non-COM object!");
		this->_configureSwapChain(width, height);
	}

	void DirectX12_RenderSystem::_resizeSwapChain(int width, int height)
	{
		// If the swap chain already exists, resize it.
		HRESULT hr = this->swapChain->ResizeBuffers(FRAME_COUNT, width, height, DXGI_FORMAT_B8G8R8A8_UNORM, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			//m_deviceRemoved = true;

			// Do not continue execution of this method. DeviceResources will be destroyed and re-created.
			return;
		}
		_TRY_UNSAFE(hr, "Unable to resize swap chain buffers!");
		this->_configureSwapChain(width, height);
	}

	void DirectX12_RenderSystem::_configureSwapChain(int width, int height)
	{
		_TRY_UNSAFE(this->swapChain->SetRotation(this->_getDxgiRotation()), "Unable to set rotation on swap chain!");
		this->currentFrame = this->swapChain->GetCurrentBackBufferIndex();
		D3D12_CPU_DESCRIPTOR_HANDLE rtvDesc = this->rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for_iter (i, 0, FRAME_COUNT)
		{
			_TRY_UNSAFE(this->swapChain->GetBuffer(i, IID_PPV_ARGS(&this->renderTargets[i])), hsprintf("Unable to get buffer %d from swap chain!", i));
			this->d3dDevice->CreateRenderTargetView(this->renderTargets[i].Get(), nullptr, rtvDesc);
			rtvDesc.ptr += this->rtvDescSize;
		}
		if (this->options.depthBuffer)
		{
			D3D12_HEAP_PROPERTIES depthHeapProperties = {};
			depthHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
			depthHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			depthHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			depthHeapProperties.CreationNodeMask = 1;
			depthHeapProperties.VisibleNodeMask = 1;
			D3D12_RESOURCE_DESC depthResourceDesc = {};
			depthResourceDesc.Format = DXGI_FORMAT_D16_UNORM;
			depthResourceDesc.Alignment = 0;
			depthResourceDesc.Width = width;
			depthResourceDesc.Height = height;
			depthResourceDesc.DepthOrArraySize = 1;
			depthResourceDesc.MipLevels = 1;
			depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			depthResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			depthResourceDesc.SampleDesc.Count = 1;
			depthResourceDesc.SampleDesc.Quality = 0;
			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			float depth = 1.0f;
			depthOptimizedClearValue.Format = DXGI_FORMAT_D16_UNORM;
			// using memcpy to preserve NAN values
			memcpy(&depthOptimizedClearValue.DepthStencil.Depth, &depth, sizeof(depth));
			depthOptimizedClearValue.DepthStencil.Stencil = 0;
			_TRY_UNSAFE(this->d3dDevice->CreateCommittedResource(&depthHeapProperties, D3D12_HEAP_FLAG_NONE, &depthResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue, IID_PPV_ARGS(&this->depthStencil)), "Unable to create depth buffer!");
			D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D16_UNORM;
			dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
			this->d3dDevice->CreateDepthStencilView(this->depthStencil.Get(), &dsvDesc, this->dsvHeap->GetCPUDescriptorHandleForHeapStart());
		}
		//this->screenViewport = { 0.0f, 0.0f, this->d3dRenderTargetSize.Width, this->d3dRenderTargetSize.Height, 0.0f, 1.0f };
	}

	int DirectX12_RenderSystem::getVRam() const
	{
		if (this->d3dDevice == nullptr)
		{
			return 0;
		}
		ComPtr<IDXGIDevice2> dxgiDevice;
		HRESULT hr = this->d3dDevice.As(&dxgiDevice);
		if (FAILED(hr))
		{
			hlog::error(logTag, "Unable to retrieve DXGI device!");
			return 0;
		}
		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		if (FAILED(hr))
		{
			hlog::error(logTag, "Unable to get adapter from DXGI device!");
			return 0;
		}
		DXGI_ADAPTER_DESC desc;
		hr = dxgiAdapter->GetDesc(&desc);
		if (FAILED(hr))
		{
			hlog::error(logTag, "Unable to get description from DXGI adapter!");
			return 0;
		}
		return (desc.DedicatedVideoMemory / (1024 * 1024));
	}

	Texture* DirectX12_RenderSystem::_deviceCreateTexture(bool fromResource)
	{
		return new DirectX12_Texture(fromResource);
	}

	PixelShader* DirectX12_RenderSystem::_deviceCreatePixelShader()
	{
		return new DirectX12_PixelShader();
	}

	VertexShader* DirectX12_RenderSystem::_deviceCreateVertexShader()
	{
		return new DirectX12_VertexShader();
	}

	void DirectX12_RenderSystem::_deviceChangeResolution(int w, int h, bool fullscreen)
	{
		if (this->swapChain != nullptr)
		{
			this->_resizeSwapChain(april::window->getWidth(), april::window->getHeight());
		}
		else
		{
			this->_createSwapChain(april::window->getWidth(), april::window->getHeight());
		}
	}

	void DirectX12_RenderSystem::_setDeviceViewport(cgrect rect)
	{
		grect viewport = rect;
		// this is needed on WinRT because of a graphics driver bug on Windows RT and on WinP8 because of a completely different graphics driver bug on Windows Phone 8
		gvec2 resolution = april::getSystemInfo().displayResolution;
		int w = april::window->getWidth();
		int h = april::window->getHeight();
		if (viewport.x < 0.0f)
		{
			viewport.w += viewport.x;
			viewport.x = 0.0f;
		}
		if (viewport.y < 0.0f)
		{
			viewport.h += viewport.y;
			viewport.y = 0.0f;
		}
		viewport.w = hclamp(viewport.w, 0.0f, hmax(w - viewport.x, 0.0f));
		viewport.h = hclamp(viewport.h, 0.0f, hmax(h - viewport.y, 0.0f));
		if (viewport.w > 0.0f && viewport.h > 0.0f)
		{
			viewport.x = hclamp(viewport.x, 0.0f, (float)w);
			viewport.y = hclamp(viewport.y, 0.0f, (float)h);
		}
		else
		{
			viewport.set((float)w, (float)h, 0.0f, 0.0f);
		}
		// setting the system viewport
		D3D12_VIEWPORT dx12Viewport;
		dx12Viewport.MinDepth = D3D12_MIN_DEPTH;
		dx12Viewport.MaxDepth = D3D12_MAX_DEPTH;
		// these double-casts are to ensure consistent behavior among rendering systems
		dx12Viewport.TopLeftX = (float)(int)viewport.x;
		dx12Viewport.TopLeftY = (float)(int)viewport.y;
		dx12Viewport.Width = (float)(int)viewport.w;
		dx12Viewport.Height = (float)(int)viewport.h;
		// TODOuwp
		//this->d3dDeviceContext->RSSetViewports(1, &dx12Viewport);
	}

	void DirectX12_RenderSystem::_setDeviceModelviewMatrix(const gmat4& matrix)
	{
		this->deviceState_constantBufferChanged = true;
	}

	void DirectX12_RenderSystem::_setDeviceProjectionMatrix(const gmat4& matrix)
	{
		this->deviceState_constantBufferChanged = true;
	}

	void DirectX12_RenderSystem::_setDeviceDepthBuffer(bool enabled, bool writeEnabled)
	{
		hlog::error(logTag, "Not implemented!");
	}

	void DirectX12_RenderSystem::_setDeviceRenderMode(bool useTexture, bool useColor)
	{
		// not used
	}

	void DirectX12_RenderSystem::_setDeviceTexture(Texture* texture)
	{
		// not used
	}

	void DirectX12_RenderSystem::_setDeviceTextureFilter(const Texture::Filter& textureFilter)
	{
		// not used
	}

	void DirectX12_RenderSystem::_setDeviceTextureAddressMode(const Texture::AddressMode& textureAddressMode)
	{
		// not used
	}

	void DirectX12_RenderSystem::_setDeviceBlendMode(const BlendMode& blendMode)
	{
		// not used
	}

	void DirectX12_RenderSystem::_setDeviceColorMode(const ColorMode& colorMode, float colorModeFactor, bool useTexture, bool useColor, const Color& systemColor)
	{
		this->deviceState_constantBufferChanged = true;
	}

	void DirectX12_RenderSystem::_deviceClear(bool depth)
	{
		static const float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		D3D12_CPU_DESCRIPTOR_HANDLE handle = this->rtvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += this->currentFrame * this->rtvDescSize;
		this->commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
	}
	
	void DirectX12_RenderSystem::_deviceClear(const Color& color, bool depth)
	{
		static float clearColor[4] = {0.0f, 0.0f, 0.0f, 0.0f};
		clearColor[0] = color.b_f();
		clearColor[1] = color.g_f();
		clearColor[2] = color.r_f();
		clearColor[3] = color.a_f();
		D3D12_CPU_DESCRIPTOR_HANDLE handle = this->rtvHeap->GetCPUDescriptorHandleForHeapStart();
		handle.ptr += this->currentFrame * this->rtvDescSize;
		this->commandList->ClearRenderTargetView(handle, clearColor, 0, nullptr);
	}

	void DirectX12_RenderSystem::_deviceClearDepth()
	{
		static const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
		this->commandList->ClearDepthStencilView(this->dsvHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
	}

	void DirectX12_RenderSystem::_deviceRender(const RenderOperation& renderOperation, const PlainVertex* vertices, int count)
	{
		this->_renderDX12VertexBuffer(renderOperation, vertices, count, sizeof(PlainVertex));
	}

	void DirectX12_RenderSystem::_deviceRender(const RenderOperation& renderOperation, const TexturedVertex* vertices, int count)
	{
		this->_renderDX12VertexBuffer(renderOperation, vertices, count, sizeof(TexturedVertex));
	}

	void DirectX12_RenderSystem::_deviceRender(const RenderOperation& renderOperation, const ColoredVertex* vertices, int count)
	{
		this->_renderDX12VertexBuffer(renderOperation, vertices, count, sizeof(ColoredVertex));
	}

	void DirectX12_RenderSystem::_deviceRender(const RenderOperation& renderOperation, const ColoredTexturedVertex* vertices, int count)
	{
		this->_renderDX12VertexBuffer(renderOperation, vertices, count, sizeof(ColoredTexturedVertex));
	}

	void DirectX12_RenderSystem::_updatePipelineState(const RenderOperation& renderOperation)
	{
		int r = 0;
		int i = 0;
		int j = this->state->colorMode.value;
		int k = this->state->blendMode.value;
		int l = 0;
		int m = 0;
		if (this->state->useTexture)
		{
			i += 2;
			++r;
		}
		if (this->state->useColor)
		{
			++i;
		}
		if (renderOperation.isLine())
		{
			++l;
		}
		else if (renderOperation.isPoint())
		{
			l += 2;
		}
		if (this->state->depthBuffer)
		{
			++m;
		}
		bool changed = false;
		if (this->deviceState_constantBufferChanged || this->deviceState_pipelineState != this->pipelineStates[i][j][k][l][m])
		{
			changed = true;
		}
		if (!changed)
		{
			return;
		}
		this->deviceState_pipelineState = this->pipelineStates[i][j][k][l][m];
		this->deviceState_rootSignature = this->rootSignatures[r];
		this->executeCurrentCommands();
		this->waitForCommands();
		this->prepareNewCommands();
		if (this->deviceState_constantBufferChanged)
		{
			this->constantBufferData.matrix = (this->deviceState->projectionMatrix * this->deviceState->modelviewMatrix).transposed();
			this->constantBufferData.systemColor.set(this->deviceState->systemColor.r_f(), this->deviceState->systemColor.g_f(),
				this->deviceState->systemColor.b_f(), this->deviceState->systemColor.a_f());
			this->constantBufferData.lerpAlpha.set(this->deviceState->colorModeFactor, this->deviceState->colorModeFactor,
				this->deviceState->colorModeFactor, this->deviceState->colorModeFactor);
			unsigned char* mappedConstantBuffer = this->mappedConstantBuffer + (this->currentFrame * ALIGNED_CONSTANT_BUFFER_SIZE);
			memcpy(mappedConstantBuffer, &this->constantBufferData, sizeof(ConstantBuffer));
			this->deviceState_constantBufferChanged = false;
		}
	}

	void DirectX12_RenderSystem::executeCurrentCommands()
	{
		PIXEndEvent(this->commandList.Get());
		this->commandList->Close();
		ID3D12CommandList* ppCommandLists[] = { this->commandList.Get() };
		this->commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void DirectX12_RenderSystem::waitForCommands()
	{
		const UINT64 currentFenceValue = this->fenceValues[this->currentFrame];
		_TRY_UNSAFE(this->commandQueue->Signal(this->fence.Get(), currentFenceValue), "Unable to signal command queue!");
		this->currentFrame = this->swapChain->GetCurrentBackBufferIndex();
		if (this->fence->GetCompletedValue() < this->fenceValues[this->currentFrame])
		{
			_TRY_UNSAFE(this->fence->SetEventOnCompletion(this->fenceValues[this->currentFrame], this->fenceEvent), "Unable to set even on completion!");
			WaitForSingleObjectEx(this->fenceEvent, INFINITE, FALSE);
		}
		this->fenceValues[this->currentFrame] = currentFenceValue + 1;
	}

	void DirectX12_RenderSystem::prepareNewCommands()
	{
		_TRY_UNSAFE(this->commandAllocators[this->currentFrame]->Reset(), hsprintf("Unable to reset command allocator %d!", this->currentFrame));
		_TRY_UNSAFE(this->commandList->Reset(this->commandAllocators[this->currentFrame].Get(), this->deviceState_pipelineState.Get()), "Unable to reset command list!");
		PIXBeginEvent(this->commandList.Get(), 0, L"");
		this->commandList->SetGraphicsRootSignature(this->deviceState_rootSignature.Get());
		ID3D12DescriptorHeap* heaps[] = { this->cbvHeap.Get() };
		this->commandList->SetDescriptorHeaps(_countof(heaps), heaps);
		CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(this->cbvHeap->GetGPUDescriptorHandleForHeapStart(), this->currentFrame, this->cbvDescSize);
		this->commandList->SetGraphicsRootDescriptorTable(0, gpuHandle);
	}

	void DirectX12_RenderSystem::_renderDX12VertexBuffer(const RenderOperation& renderOperation, const void* data, int count, unsigned int vertexSize)
	{
		this->_updatePipelineState(renderOperation);
		D3D12_SUBRESOURCE_DATA vertexData = {};
		vertexData.pData = data;
		vertexData.RowPitch = (count * vertexSize);
		vertexData.SlicePitch = vertexData.RowPitch;
		UpdateSubresources(this->commandList.Get(), this->vertexBuffers[this->vertexBufferIndex].Get(), this->vertexBufferUploads[this->vertexBufferIndex].Get(), 0, 0, 1, &vertexData);
		
		D3D12_RESOURCE_BARRIER vertexBufferResourceBarrier = {};
		vertexBufferResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		vertexBufferResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        vertexBufferResourceBarrier.Transition.pResource = this->vertexBuffers[this->vertexBufferIndex].Get();
        vertexBufferResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        vertexBufferResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        vertexBufferResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		this->commandList->ResourceBarrier(1, &vertexBufferResourceBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE renderTargetView = this->rtvHeap->GetCPUDescriptorHandleForHeapStart();
		renderTargetView.ptr += this->currentFrame * this->rtvDescSize;
		if (this->deviceState->useTexture && this->deviceState->texture != NULL)
		{
			this->commandList->SetGraphicsRootDescriptorTable(1, ((DirectX12_Texture*)this->deviceState->texture)->srvHeap->GetGPUDescriptorHandleForHeapStart());
		}


		grect viewport = this->getViewport();
		// Set the viewport and scissor rectangle.
		D3D12_VIEWPORT dx12Viewport;
		dx12Viewport.MinDepth = D3D12_MIN_DEPTH;
		dx12Viewport.MaxDepth = D3D12_MAX_DEPTH;
		// these double-casts are to ensure consistent behavior among rendering systems
		dx12Viewport.TopLeftX = (float)(int)viewport.x;
		dx12Viewport.TopLeftY = (float)(int)viewport.y;
		dx12Viewport.Width = (float)(int)viewport.w;
		dx12Viewport.Height = (float)(int)viewport.h;

		this->commandList->RSSetViewports(1, &dx12Viewport);
		D3D12_RECT scissorRect = { 0, 0, static_cast<LONG>(dx12Viewport.Width), static_cast<LONG>(dx12Viewport.Height) };
		this->commandList->RSSetScissorRects(1, &scissorRect);

		D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = this->dsvHeap->GetCPUDescriptorHandleForHeapStart();
		//this->commandList->OMSetRenderTargets(1, &renderTargetView, false, &depthStencilView);
		this->commandList->OMSetRenderTargets(1, &renderTargetView, false, NULL);
		
		this->commandList->IASetPrimitiveTopology(_dx12RenderOperations[renderOperation.value]);
		this->vertexBufferViews[this->vertexBufferIndex].BufferLocation = this->vertexBuffers[this->vertexBufferIndex]->GetGPUVirtualAddress();
		this->vertexBufferViews[this->vertexBufferIndex].StrideInBytes = vertexSize;
		this->vertexBufferViews[this->vertexBufferIndex].SizeInBytes = count * vertexSize;

		this->commandList->IASetVertexBuffers(0, 1, &this->vertexBufferViews[this->vertexBufferIndex]);
		this->commandList->DrawInstanced(count, 1, 0, 0);

		this->vertexBufferIndex = (this->vertexBufferIndex + 1) % MAX_VERTEX_BUFFERS;
	}

	Image::Format DirectX12_RenderSystem::getNativeTextureFormat(Image::Format format) const
	{
		if (format == Image::Format::RGBA || format == Image::Format::ARGB || format == Image::Format::BGRA || format == Image::Format::ABGR)
		{
			return Image::Format::RGBA;
		}
		if (format == Image::Format::RGBX || format == Image::Format::XRGB || format == Image::Format::BGRX ||
			format == Image::Format::XBGR || format == Image::Format::RGB || format == Image::Format::BGR)
		{
			return Image::Format::RGBX;
		}
		if (format == Image::Format::Alpha || format == Image::Format::Greyscale || format == Image::Format::Palette)
		{
			return format;
		}
		return Image::Format::Invalid;
	}
	
	unsigned int DirectX12_RenderSystem::getNativeColorUInt(const april::Color& color) const
	{
		return ((color.a << 24) | (color.b << 16) | (color.g << 8) | color.r);
	}

	Image* DirectX12_RenderSystem::takeScreenshot(Image::Format format)
	{
		// TODOa - if possible
		hlog::warn(logTag, "DirectX12_RenderSystem::takeScreenshot() not implemented!");
		return NULL;
	}
	
	void DirectX12_RenderSystem::presentFrame()
	{
		D3D12_RESOURCE_BARRIER presentResourceBarrier = {};
		presentResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		presentResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		presentResourceBarrier.Transition.pResource = this->renderTargets[this->currentFrame].Get();
		presentResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		presentResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		presentResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		this->commandList->ResourceBarrier(1, &presentResourceBarrier);
		this->executeCurrentCommands();
		HRESULT hr = this->swapChain->Present(1, 0);
		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			//m_deviceRemoved = true;
			return;
		}
		_TRY_UNSAFE(hr, "Unable to present swap chain!");
		this->deviceState_rootSignature = (!this->state->useTexture ? this->rootSignatures[0] : this->rootSignatures[1]);
		this->waitForCommands();
		this->prepareNewCommands();
		D3D12_RESOURCE_BARRIER renderTargetResourceBarrier = {};
		renderTargetResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		renderTargetResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		renderTargetResourceBarrier.Transition.pResource = this->renderTargets[this->currentFrame].Get();
		renderTargetResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		renderTargetResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		renderTargetResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		this->commandList->ResourceBarrier(1, &renderTargetResourceBarrier);
		this->vertexBufferIndex = 0;
	}

	void DirectX12_RenderSystem::updateDeviceReset()
	{
		// TODOuwp
		/*
		if (this->deviceRemoved)
		{
			// do stuff
		}
		*/
	}

	void DirectX12_RenderSystem::updateWindowSize(bool reconfigureIfChanged)
	{
		this->coreWindow = CoreWindow::GetForCurrentThread();
		DisplayInformation^ displayInformation = DisplayInformation::GetForCurrentView();
		this->nativeOrientation = displayInformation->NativeOrientation;
		if (reconfigureIfChanged)
		{
			bool changed = false;
			Size newLogicalSize(this->coreWindow->Bounds.Width, this->coreWindow->Bounds.Height);
			if (this->logicalSize != newLogicalSize)
			{
				this->logicalSize != newLogicalSize;
				changed = true;
			}
			if (this->currentOrientation != displayInformation->CurrentOrientation)
			{
				this->currentOrientation = displayInformation->CurrentOrientation;
				changed = true;
			}
			if (this->dpi != displayInformation->LogicalDpi)
			{
				this->dpi = displayInformation->LogicalDpi;
				changed = true;
			}
			if (changed)
			{
				this->_configureDevice();
			}
		}
		else
		{
			this->logicalSize = Windows::Foundation::Size(this->coreWindow->Bounds.Width, this->coreWindow->Bounds.Height);
			this->currentOrientation = displayInformation->CurrentOrientation;
			this->dpi = displayInformation->LogicalDpi;
		}
	}

	DXGI_MODE_ROTATION DirectX12_RenderSystem::_getDxgiRotation() const
	{
		switch (this->nativeOrientation)
		{
		case DisplayOrientations::Landscape:
			switch (this->currentOrientation)
			{
			case DisplayOrientations::Landscape:		return DXGI_MODE_ROTATION_IDENTITY;
			case DisplayOrientations::Portrait:			return DXGI_MODE_ROTATION_ROTATE270;
			case DisplayOrientations::LandscapeFlipped:	return DXGI_MODE_ROTATION_ROTATE180;
			case DisplayOrientations::PortraitFlipped:	return DXGI_MODE_ROTATION_ROTATE90;
			}
			break;
		case DisplayOrientations::Portrait:
			switch (this->currentOrientation)
			{
			case DisplayOrientations::Landscape:		return DXGI_MODE_ROTATION_ROTATE90;
			case DisplayOrientations::Portrait:			return DXGI_MODE_ROTATION_IDENTITY;
			case DisplayOrientations::LandscapeFlipped:	return DXGI_MODE_ROTATION_ROTATE270;
			case DisplayOrientations::PortraitFlipped:	return DXGI_MODE_ROTATION_ROTATE180;
			}
			break;
		}
		return DXGI_MODE_ROTATION_UNSPECIFIED;
	}

	Texture* DirectX12_RenderSystem::getRenderTarget()
	{
		// TODOa - implement
		return NULL;// this->renderTarget;
	}

	void DirectX12_RenderSystem::setRenderTarget(Texture* source)
	{
		// TODOa - implement (this code is experimental)
		/*
		DirectX12_Texture* texture = (DirectX12_Texture*)source;
		if (texture == NULL)
		{
			// has to use GetAddressOf(), because the parameter is a pointer to an array of render target views
			this->d3dDeviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), NULL);
		}
		else if (texture->d3dRenderTargetView != nullptr)
		{
			// has to use GetAddressOf(), because the parameter is a pointer to an array of render target views
			this->d3dDeviceContext->OMSetRenderTargets(1, texture->d3dRenderTargetView.GetAddressOf(), NULL);
		}
		else
		{
			hlog::error(logTag, "Texture not created as rendertarget: " + texture->_getInternalName());
			return;
		}
		this->renderTarget = texture;
		*/
		this->deviceState_constantBufferChanged = true;
	}

	void DirectX12_RenderSystem::setPixelShader(PixelShader* pixelShader)
	{
		// TODOa
		//this->activePixelShader = (DirectX12_PixelShader*)pixelShader;
	}

	void DirectX12_RenderSystem::setVertexShader(VertexShader* vertexShader)
	{
		// TODOa
		//this->activeVertexShader = (DirectX12_VertexShader*)vertexShader;
	}

}

#endif
