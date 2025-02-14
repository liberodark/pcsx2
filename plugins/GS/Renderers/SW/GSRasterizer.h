/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#include "../../GS.h"
#include "GSVertexSW.h"
#include "../../GSAlignedClass.h"
#include "../../GSThread_CXX11.h"

class alignas(32) GSRasterizerData : public GSAlignedClass<32>
{
	static int s_counter;

public:
	GSVector4i scissor;
	GSVector4i bbox;
	GS_PRIM_CLASS primclass;
	uint8* buff;
	GSVertexSW* vertex;
	int vertex_count;
	uint32* index;
	int index_count;
	uint64 frame;
	int pixels;
	int counter;

	GSRasterizerData() 
		: scissor(GSVector4i::zero())
		, bbox(GSVector4i::zero())
		, primclass(GS_INVALID_CLASS)
		, buff(NULL)
		, vertex(NULL)
		, vertex_count(0)
		, index(NULL)
		, index_count(0)
		, frame(0)
		, pixels(0)
	{
		counter = s_counter++;
	}

	virtual ~GSRasterizerData() 
	{
		if(buff != NULL) _aligned_free(buff);
	}
};

class IDrawScanline : public GSAlignedClass<32>
{
public:
	typedef void (*SetupPrimPtr)(const GSVertexSW* vertex, const uint32* index, const GSVertexSW& dscan);
	typedef void (__fastcall *DrawScanlinePtr)(int pixels, int left, int top, const GSVertexSW& scan);
	typedef void (IDrawScanline::*DrawRectPtr)(const GSVector4i& r, const GSVertexSW& v); // TODO: jit

protected:
	SetupPrimPtr m_sp;
	DrawScanlinePtr m_ds;
	DrawScanlinePtr m_de;
	DrawRectPtr m_dr;

public:
	IDrawScanline() : m_sp(NULL), m_ds(NULL), m_de(NULL), m_dr(NULL) {}
	virtual ~IDrawScanline() {}

	virtual void BeginDraw(const GSRasterizerData* data) = 0;
	virtual void EndDraw(uint64 frame, int actual, int total) = 0;

#ifdef ENABLE_JIT_RASTERIZER

	__forceinline void SetupPrim(const GSVertexSW* vertex, const uint32* index, const GSVertexSW& dscan) {m_sp(vertex, index, dscan);}
	__forceinline void DrawScanline(int pixels, int left, int top, const GSVertexSW& scan) {m_ds(pixels, left, top, scan);}
	__forceinline void DrawEdge(int pixels, int left, int top, const GSVertexSW& scan) {m_de(pixels, left, top, scan);}
	__forceinline void DrawRect(const GSVector4i& r, const GSVertexSW& v) {(this->*m_dr)(r, v);}

#else

	virtual void SetupPrim(const GSVertexSW* vertex, const uint32* index, const GSVertexSW& dscan) = 0;
	virtual void DrawScanline(int pixels, int left, int top, const GSVertexSW& scan) = 0;
	virtual void DrawEdge(int pixels, int left, int top, const GSVertexSW& scan) = 0;
	virtual void DrawRect(const GSVector4i& r, const GSVertexSW& v) = 0;
	
#endif

	__forceinline bool HasEdge() const {return m_de != NULL;}
	__forceinline bool IsSolidRect() const {return m_dr != NULL;}
};

class IRasterizer : public GSAlignedClass<32>
{
public:
	virtual ~IRasterizer() {}

	virtual void Queue(const std::shared_ptr<GSRasterizerData>& data) = 0;
	virtual void Sync() = 0;
	virtual bool IsSynced() const = 0;
	virtual int GetPixels(bool reset = true) = 0;
};

class alignas(32) GSRasterizer : public IRasterizer
{
protected:
	IDrawScanline* m_ds;
	int m_id;
	int m_threads;
	int m_thread_height;
	uint8* m_scanline;
	GSVector4i m_scissor;
	GSVector4 m_fscissor_x;
	GSVector4 m_fscissor_y;
	struct {GSVertexSW* buff; int count;} m_edge;
	struct {int sum, actual, total;} m_pixels;

	typedef void (GSRasterizer::*DrawPrimPtr)(const GSVertexSW* v, int count);

	template<bool scissor_test>
	void DrawPoint(const GSVertexSW* vertex, int vertex_count, const uint32* index, int index_count);
	void DrawLine(const GSVertexSW* vertex, const uint32* index);
	void DrawTriangle(const GSVertexSW* vertex, const uint32* index);
	void DrawSprite(const GSVertexSW* vertex, const uint32* index);

	#if _M_SSE >= 0x501
	__forceinline void DrawTriangleSection(int top, int bottom, GSVertexSW2& edge, const GSVertexSW2& dedge, const GSVertexSW2& dscan, const GSVector4& p0);
	#else
	__forceinline void DrawTriangleSection(int top, int bottom, GSVertexSW& edge, const GSVertexSW& dedge, const GSVertexSW& dscan, const GSVector4& p0);
	#endif

	void DrawEdge(const GSVertexSW& v0, const GSVertexSW& v1, const GSVertexSW& dv, int orientation, int side);

	__forceinline void AddScanline(GSVertexSW* e, int pixels, int left, int top, const GSVertexSW& scan);
	__forceinline void Flush(const GSVertexSW* vertex, const uint32* index, const GSVertexSW& dscan, bool edge = false);

	__forceinline void DrawScanline(int pixels, int left, int top, const GSVertexSW& scan);
	__forceinline void DrawEdge(int pixels, int left, int top, const GSVertexSW& scan);

public:
	GSRasterizer(IDrawScanline* ds, int id, int threads);
	virtual ~GSRasterizer();

	__forceinline bool IsOneOfMyScanlines(int top) const;
	__forceinline bool IsOneOfMyScanlines(int top, int bottom) const;
	__forceinline int FindMyNextScanline(int top) const;

	void Draw(GSRasterizerData* data);

	// IRasterizer

	void Queue(const std::shared_ptr<GSRasterizerData>& data);
	void Sync() {}
	bool IsSynced() const {return true;}
	int GetPixels(bool reset);
};

class GSRasterizerList : public IRasterizer
{
protected:
	using GSWorker = GSJobQueue<std::shared_ptr<GSRasterizerData>, 65536>;

	// Worker threads depend on the rasterizers, so don't change the order.
	std::vector<std::unique_ptr<GSRasterizer>> m_r;
	std::vector<std::unique_ptr<GSWorker>> m_workers;
	uint8* m_scanline;
	int m_thread_height;

	GSRasterizerList(int threads);

public:
	virtual ~GSRasterizerList();

	template<class DS> static IRasterizer* Create(int threads)
	{
		threads = std::max<int>(threads, 0);

		if(threads == 0)
		{
			return new GSRasterizer(new DS(), 0, 1);
		}

		GSRasterizerList* rl = new GSRasterizerList(threads);

		for(int i = 0; i < threads; i++)
		{
			rl->m_r.push_back(std::unique_ptr<GSRasterizer>(new GSRasterizer(new DS(), i, threads)));
			auto &r = *rl->m_r[i];
			rl->m_workers.push_back(std::unique_ptr<GSWorker>(new GSWorker(
				[&r](std::shared_ptr<GSRasterizerData> &item) { r.Draw(item.get()); })));
		}

		return rl;
	}

	// IRasterizer

	void Queue(const std::shared_ptr<GSRasterizerData>& data);
	void Sync();
	bool IsSynced() const;
	int GetPixels(bool reset);
};
