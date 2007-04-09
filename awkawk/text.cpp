//  Copyright (C) 2006 Peter Bright
//  
//  This software is provided 'as-is', without any express or implied
//  warranty.  In no event will the authors be held liable for any damages
//  arising from the use of this software.
//  
//  Permission is granted to anyone to use this software for any purpose,
//  including commercial applications, and to alter it and redistribute it
//  freely, subject to the following restrictions:
//  
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//  2. Altered source versions must be plainly marked as such, and must not be
//     misrepresented as being the original software.
//  3. This notice may not be removed or altered from any source distribution.
//  
//  Peter Bright <drpizza@quiscalusmexicanus.org>

#include "stdafx.h"

#include "text.h"
#include "util.h"

#include <vector>
#include <cmath>
#include <gl/glu.h>
#pragma comment(lib, "glu32.lib")

namespace
{

struct contour : public std::vector<vec2d>
{
	vec2d EdgeNormal(int vertexNum) const
	{
		return normal(GetVertexCE(vertexNum + 1) - GetVertexCE(vertexNum)).normalize();
	}

	vec2d VertexNormal(int vertexNum) const
	{
		vec2d v1(GetVertexCE(vertexNum - 1) - GetVertexCE(vertexNum));
		vec2d v2(GetVertexCE(vertexNum + 1) - GetVertexCE(vertexNum));
		return (normal(v1).normalize() + (normal(v2).normalize() * -1.0)).normalize() * -1.0;
	}

	double VertexCosAngle(int vertexNum) const
	{
		vec2d v1(GetVertexCE(vertexNum - 1) - GetVertexCE(vertexNum));
		vec2d v2(GetVertexCE(vertexNum + 1) - GetVertexCE(vertexNum));
		return cos_angle(v1, v2);
	}

	vec2d GetVertexCE(int vertexNum) const
	{
		if(vertexNum < 0)
		{
			vertexNum = static_cast<int>(size()) - (-vertexNum % static_cast<int>(size()));
		}
		return at(vertexNum % size());
	}
};

struct PathPolygon : public std::vector<contour>
{
	PathPolygon(HDC dc)
	{
		// convert Bézier curves in the path to straight lines
		::FlattenPath(dc);

		// determine the number of endpoints in the path
		const int num_points(::GetPath(dc, NULL, NULL, 0));
		boost::scoped_array<POINT> points(new POINT[num_points]);
		boost::scoped_array<BYTE> types(new BYTE[num_points]);

		// get the path's description
		::GetPath(dc, points.get(), types.get(), num_points);

		contour cont;
		vec2d vect;

		for(int i(0); i < num_points; ++i)
		{
			vect[0] = points[i].x;
			vect[1] = points[i].y;
			cont.push_back(vect);
			if(types[i] & PT_CLOSEFIGURE)
			{
				push_back(cont);
				cont.clear();
			}
		}
	}
};

struct Vertex
{
	vec3d position;
	vec3d normal;
	double tu, tv;

	Vertex()
	{
		normal[2] = 1.0f;
	}
};

inline std::vector<Vertex> generate_vertices(const PathPolygon& pathPolygon)
{
	std::vector<Vertex> vertices;
	double min_x(pathPolygon[0][0][0]);
	double max_x(min_x);
	double min_y(-pathPolygon[0][0][1]);
	double max_y(min_y);

	// go through the contours
	for(size_t i(0); i < pathPolygon.size(); ++i)
	{
		const contour& r_contour(pathPolygon[i]);

		for(size_t p(0); p < r_contour.size(); ++p)
		{
			// Add this point of the contour to the list of vertices.
			// Flip the vertices because the PathPolygon has a different
			// coordinate space than a normal Direct3D application.
			Vertex flippedVert;
			flippedVert.position[0] = r_contour[p][0];
			flippedVert.position[1] = -r_contour[p][1];
			flippedVert.normal[2] = 1.0;
			vertices.push_back(flippedVert);

			if(flippedVert.position[0] < min_x)
			{
				min_x = flippedVert.position[0];
			}
			if(flippedVert.position[0] > max_x)
			{
				max_x = flippedVert.position[0];
			}
			if(flippedVert.position[1] < min_y)
			{
				min_y = flippedVert.position[1];
			}
			if(flippedVert.position[1] > max_y)
			{
				max_y = flippedVert.position[1];
			}
		}
	}

	// calculate texture coordinates
	const double u_per_x(1.0 / (max_x - min_x));
	const double v_per_y(1.0 / (max_y - min_y));

	for(size_t v(0); v < vertices.size(); ++v)
	{
		Vertex& r_vert(vertices[v]);
		r_vert.tu = (r_vert.position[0] - min_x) * u_per_x;
		r_vert.tv = 1.0 - (r_vert.position[1] - min_y) * v_per_y;
	}
	return vertices;
}

inline bool sharp_edge(double cosAngle)
{
	if(cosAngle >= -0.7071068 && cosAngle <= 1.0)
	{
		return true;
	}
	else
	{
		return false;
	}
}

struct PathExtruder
{
	std::vector<Vertex> vertices;
	std::vector<size_t> indices;

	PathExtruder(const PathPolygon& pathPolygon, float extrusion) : vertices(generate_vertices(pathPolygon))
	{
		// create tesselation object
		GLUtesselator* tess(gluNewTess());
		if(tess == NULL)
		{
			throw std::runtime_error("Creation of tesselation object failed.");
		}
		ON_BLOCK_EXIT(&::gluDeleteTess, tess);

		// register tesselation callbacks
		typedef void (__stdcall *GluTessCallbackType)();
		gluTessCallback(tess, GLU_TESS_BEGIN_DATA, reinterpret_cast<GluTessCallbackType>(BeginCB));
		gluTessCallback(tess, GLU_TESS_EDGE_FLAG_DATA, reinterpret_cast<GluTessCallbackType>(EdgeFlagCB));
		gluTessCallback(tess, GLU_TESS_VERTEX_DATA, reinterpret_cast<GluTessCallbackType>(VertexCB));
		gluTessCallback(tess, GLU_TESS_END_DATA, reinterpret_cast<GluTessCallbackType>(EndCB));
		gluTessCallback(tess, GLU_TESS_COMBINE_DATA, reinterpret_cast<GluTessCallbackType>(CombineCB));
		gluTessCallback(tess, GLU_TESS_ERROR_DATA, reinterpret_cast<GluTessCallbackType>(ErrorCB));

		// begin polygon
		gluTessBeginPolygon(tess, reinterpret_cast<void*>(this));

		size_t vertex_num(0);

		// go through the contours
		for(size_t i(0); i < pathPolygon.size(); ++i)
		{
			gluTessBeginContour(tess);
			const contour& r_contour(pathPolygon[i]);
			for(size_t p(0); p < r_contour.size(); ++p)
			{
				// pass the corresponding vertex to the tesselator object
				gluTessVertex(tess, reinterpret_cast<double*>(&vertices[vertex_num]), reinterpret_cast<void*>(vertex_num));
				++vertex_num;
			}
			gluTessEndContour(tess);
		}

		// end polygon
		gluTessEndPolygon(tess);

		// create back side
		size_t num_front_side_vertices(vertices.size());
		for(size_t v(0); v < num_front_side_vertices; ++v)
		{
			Vertex new_vertex(vertices[v]);
			new_vertex.position[2] += extrusion;
			new_vertex.normal[2] *= -1.0;
			vertices.push_back(new_vertex);
		}

		size_t num_front_side_tris(indices.size() / 3);
		for(size_t t(0); t < num_front_side_tris; ++t)
		{
			indices.push_back(indices[(t * 3) + 0] + num_front_side_vertices);
			indices.push_back(indices[(t * 3) + 2] + num_front_side_vertices);
			indices.push_back(indices[(t * 3) + 1] + num_front_side_vertices);
		}

		// create sides
		vertex_num = 0;
		for(size_t i(0); i < pathPolygon.size(); ++i)
		{
			// retrieve copy of contour
			contour r_contour(pathPolygon[i]);

			size_t first_contour_vertex(vertices.size());

			for(size_t p(0); p < r_contour.size(); ++p)
			{
				if(sharp_edge(r_contour.VertexCosAngle(p)))
				{
					Vertex new_vertex(vertices[vertex_num]);
					vec2d normal(r_contour.EdgeNormal(p - 1));
					new_vertex.normal[0] = normal[0];
					new_vertex.normal[1] = normal[1];
					new_vertex.normal[2] = 0.0;
					vertices.push_back(new_vertex);

					new_vertex = vertices[vertex_num + num_front_side_vertices];
					new_vertex.normal[0] = normal[0];
					new_vertex.normal[1] = normal[1];
					new_vertex.normal[2] = 0.0;
					vertices.push_back(new_vertex);
				}

				Vertex new_vertex(vertices[vertex_num]);
				vec2d normal(r_contour.VertexNormal(p));
				if(sharp_edge(r_contour.VertexCosAngle(p)))
				{
					normal = r_contour.EdgeNormal(p);
				}
				new_vertex.normal[0] = normal[0];
				new_vertex.normal[1] = normal[1];
				new_vertex.normal[2] = 0.0;
				vertices.push_back(new_vertex);

				new_vertex = vertices[vertex_num + num_front_side_vertices];
				new_vertex.normal[0] = normal[0];
				new_vertex.normal[1] = normal[1];
				new_vertex.normal[2] = 0.0;
				vertices.push_back(new_vertex);

				// add indices
				size_t idx_of_this_point(vertices.size() - 2); // first_contour_vertex + this_contour_vertex_offset;
				size_t idx_of_next_point((p < r_contour.size() - 1) ? idx_of_this_point + 2
				                                                    : first_contour_vertex);

				indices.push_back(idx_of_this_point);
				indices.push_back(idx_of_this_point + 1);
				indices.push_back(idx_of_next_point);

				indices.push_back(idx_of_next_point);
				indices.push_back(idx_of_this_point + 1);
				indices.push_back(idx_of_next_point + 1);

				++vertex_num;
			}
		}
	}

	virtual ~PathExtruder()
	{
	}

private:
	static void __stdcall BeginCB(GLenum type, PathExtruder* caller)
	{
	}

	static void __stdcall EdgeFlagCB(GLboolean flag, PathExtruder* caller)
	{
	}

	static void __stdcall VertexCB(size_t vertexIndex, PathExtruder* caller)
	{
		caller->indices.push_back(vertexIndex);
	}

	static void __stdcall EndCB(PathExtruder* caller)
	{
	}

	static void __stdcall CombineCB(GLdouble coords[3], size_t vertexData[4], GLfloat weight[4], size_t* outData, PathExtruder* caller)
	{
		// create a new vertex with the given coordinates
		Vertex newVertex;
		newVertex.position[0] = coords[0];
		newVertex.position[1] = coords[1];
		newVertex.position[2] = coords[2];

		const Vertex& vert0(caller->vertices[vertexData[0]]);
		const Vertex& vert1(caller->vertices[vertexData[1]]);
		const Vertex& vert2(caller->vertices[vertexData[2]]);
		const Vertex& vert3(caller->vertices[vertexData[3]]);

		newVertex.tu = (vert0.tu * weight[0]) + (vert1.tu * weight[1]) + (vert2.tu * weight[2]) + (vert3.tu * weight[3]);
		newVertex.tv = (vert0.tv * weight[0]) + (vert1.tv * weight[1]) + (vert2.tv * weight[2]) + (vert3.tv * weight[3]);

		// add the vertex to the calling object's vertex vector
		caller->vertices.push_back(newVertex);

		// pass back the index of the new vertex; it will be passed
		// as the vertexIndex parameter to VertexCB in turn
		*outData = caller->vertices.size() - 1;
	}

	static void __stdcall ErrorCB(GLenum errno, PathExtruder* caller)
	{
	}
};

}

_COM_SMARTPTR_TYPEDEF(ID3DXMesh, IID_ID3DXMesh);
_COM_SMARTPTR_TYPEDEF(ID3DXBuffer, IID_ID3DXBuffer);

HRESULT CreateTextMesh(IDirect3DDevice9* device, HDC dc, const wchar_t* text, float deviation, float extrusion, ID3DXMesh** mesh, ID3DXBuffer** adjacency)
{
	if(mesh == NULL || device == NULL || text == NULL)
	{
		return E_POINTER;
	}
	::SetBkMode(dc, TRANSPARENT);
	::BeginPath(dc);
	::TextOutW(dc, 0, 0, text, static_cast<int>(std::wcslen(text)));
	::EndPath(dc);

	PathPolygon path_polygon(dc);
	PathExtruder extruder(path_polygon, extrusion);

	static const DWORD mesh_fvf(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	struct mesh_vertex
	{
		vec3f position;
		vec3f normal;
		D3DCOLOR diffuse;
		float tu, tv;
	};

	ID3DXMeshPtr msh;
	FAIL_RET(D3DXCreateMeshFVF(static_cast<DWORD>(extruder.indices.size() / 3), static_cast<DWORD>(extruder.vertices.size()), D3DXMESH_32BIT | D3DXMESH_NPATCHES /*| D3DXMESH_WRITEONLY*/, mesh_fvf, device, &msh));
	mesh_vertex* vertices(NULL);
	FAIL_RET(msh->LockVertexBuffer(0, reinterpret_cast<void**>(&vertices)));
	for(size_t i(0); i < extruder.vertices.size(); ++i)
	{
		vertices[i].position[0] = static_cast<float>(extruder.vertices[i].position[0]);
		vertices[i].position[1] = static_cast<float>(extruder.vertices[i].position[1]);
		vertices[i].position[2] = static_cast<float>(extruder.vertices[i].position[2]);
		vertices[i].normal[0]   = static_cast<float>(extruder.vertices[i].normal[0]  );
		vertices[i].normal[1]   = static_cast<float>(extruder.vertices[i].normal[1]  );
		vertices[i].normal[2]   = static_cast<float>(extruder.vertices[i].normal[2]  );
		vertices[i].diffuse = D3DCOLOR_ARGB(0xff, 0xff, 0xff, 0xff);
		vertices[i].tu       = static_cast<float>(extruder.vertices[i].tu);
		vertices[i].tv       = static_cast<float>(extruder.vertices[i].tv);
	}
	FAIL_RET(msh->UnlockVertexBuffer());
	DWORD* indices(NULL);
	FAIL_RET(msh->LockIndexBuffer(0, reinterpret_cast<void**>(&indices)));
	for(size_t i(0); i < extruder.indices.size(); ++i)
	{
		indices[i] = static_cast<DWORD>(extruder.indices[i]);
	}
	FAIL_RET(msh->UnlockIndexBuffer());
	//D3DXComputeNormals(msh, NULL);
	ID3DXBufferPtr buffer;
	FAIL_RET(D3DXCreateBuffer(3 * msh->GetNumFaces() * sizeof(DWORD), &buffer));
	FAIL_RET(msh->GenerateAdjacency(deviation, static_cast<DWORD*>(buffer->GetBufferPointer())));
	FAIL_RET(msh->OptimizeInplace(D3DXMESHOPT_COMPACT | D3DXMESHOPT_ATTRSORT | D3DXMESHOPT_VERTEXCACHE, static_cast<const DWORD*>(buffer->GetBufferPointer()), NULL, NULL, NULL));

	*mesh = msh;
	(*mesh)->AddRef();
	if(adjacency != NULL)
	{
		ID3DXBufferPtr buffer;
		FAIL_RET(D3DXCreateBuffer(3 * msh->GetNumFaces() * sizeof(DWORD), &buffer));
		FAIL_RET(msh->GenerateAdjacency(deviation, static_cast<DWORD*>(buffer->GetBufferPointer())));
		*adjacency = buffer;
		(*adjacency)->AddRef();
	}
	return S_OK;
}
