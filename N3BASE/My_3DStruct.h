#pragma once

#include <math.h>

/* Here follow the extremely few remaining D3D8 components we require */

// NOTE: We're using this over M_PI because of the precision
#define D3DX_PI    ((FLOAT)  3.141592654f)
#define D3DXToRadian( degree ) ((degree) * (D3DX_PI / 180.0f))

/* D3D8 structs */
typedef struct _D3DVECTOR {
    float x;
    float y;
    float z;
} D3DVECTOR;

typedef struct _D3DMATRIX {
    union {
        struct {
            float        _11, _12, _13, _14;
            float        _21, _22, _23, _24;
            float        _31, _32, _33, _34;
            float        _41, _42, _43, _44;

        };
        float m[4][4];
    };
} D3DMATRIX;

struct __Matrix44;
struct __Vector3 : public D3DVECTOR // 3D Vertex
{
public:
	void	Normalize();
	float	Magnitude() const;
	float	Dot(const D3DVECTOR& vec) const;
	void	Cross(const D3DVECTOR& v1, const D3DVECTOR& v2);
	void	Absolute();

	void Zero();
	void Set(float fx, float fy, float fz);

	__Vector3() {};
	__Vector3(float fx, float fy, float fz);
	__Vector3(const D3DVECTOR& vec);

	const __Vector3& operator = (const __Vector3& vec);

	const __Vector3 operator * (const D3DMATRIX& mtx) const;
	void operator *= (float fDelta);
	void operator *= (const D3DMATRIX& mtx);
	__Vector3 operator + (const D3DVECTOR& vec) const;
	__Vector3 operator - (const D3DVECTOR& vec) const;
	__Vector3 operator * (const D3DVECTOR& vec) const;
	__Vector3 operator / (const D3DVECTOR& vec) const;

	void operator += (const D3DVECTOR& vec);
	void operator -= (const D3DVECTOR& vec);
	void operator *= (const D3DVECTOR& vec);
	void operator /= (const D3DVECTOR& vec);

	__Vector3 operator + (float fDelta) const;
	__Vector3 operator - (float fDelta) const;
	__Vector3 operator * (float fDelta) const;
	__Vector3 operator / (float fDelta) const;
};

struct __Matrix44 : public D3DMATRIX // 4x4 Matrix
{
public:
	__Matrix44() {}
	void Zero();
	void Identity();
	void RotationY(float fDelta);
};

inline void	__Vector3::Normalize()
{
	float fn = sqrtf(x*x + y*y + z*z);
	if(fn == 0) return;
	x /= fn; y /= fn; z /= fn;
}

inline float __Vector3::Magnitude() const 
{
	return sqrtf(x*x + y*y + z*z);
}

inline float __Vector3::Dot(const D3DVECTOR& vec) const 
{
	return x*vec.x + y*vec.y + z*vec.z;
}

inline void __Vector3::Cross(const D3DVECTOR& v1, const D3DVECTOR& v2)
{
	x = v1.y * v2.z - v1.z * v2.y;
	y = v1.z * v2.x - v1.x * v2.z;
	z = v1.x * v2.y - v1.y * v2.x;
}

inline void __Vector3::Absolute()
{
	if(x < 0) x *= -1.0f;
	if(y < 0) y *= -1.0f;
	if(z < 0) z *= -1.0f;
}

inline void __Vector3::Zero()
{
	x = y = z = 0;
}

inline void __Vector3::Set(float fx, float fy, float fz)
{
	x = fx; y = fy, z = fz;
}

inline __Vector3::__Vector3(float fx, float fy, float fz)
{
	x = fx; y = fy, z = fz;
}

inline __Vector3::__Vector3(const D3DVECTOR& vec)
{
	x = vec.x; y = vec.y; z = vec.z;
}

inline const __Vector3& __Vector3::operator = (const __Vector3& vec)
{
	x = vec.x; y = vec.y; z = vec.z;
	return *this;
}

inline const __Vector3 __Vector3::operator * (const D3DMATRIX& mtx) const 
{
	static __Vector3 vTmp;

	vTmp.x = x*mtx._11 + y*mtx._21 + z*mtx._31 + mtx._41;
	vTmp.y = x*mtx._12 + y*mtx._22 + z*mtx._32 + mtx._42;
	vTmp.z = x*mtx._13 + y*mtx._23 + z*mtx._33 + mtx._43;

	return vTmp;
}

inline void __Vector3::operator *= (float fDelta)
{
	x *= fDelta;
	y *= fDelta;
	z *= fDelta;
}

inline void __Vector3::operator *= (const D3DMATRIX& mtx)
{
	static __Vector3 vTmp;

	vTmp.Set(x,y,z);
	x = vTmp.x*mtx._11 + vTmp.y*mtx._21 + vTmp.z*mtx._31 + mtx._41;
	y = vTmp.x*mtx._12 + vTmp.y*mtx._22 + vTmp.z*mtx._32 + mtx._42;
	z = vTmp.x*mtx._13 + vTmp.y*mtx._23 + vTmp.z*mtx._33 + mtx._43;
}

inline __Vector3 __Vector3::operator + (const D3DVECTOR& vec) const
{
	static __Vector3 vTmp;

	vTmp.x = x + vec.x;
	vTmp.y = y + vec.y;
	vTmp.z = z + vec.z;
	return vTmp;
}

inline __Vector3 __Vector3::operator - (const D3DVECTOR& vec) const 
{
	static __Vector3 vTmp;

	vTmp.x = x - vec.x;
	vTmp.y = y - vec.y;
	vTmp.z = z - vec.z;
	return vTmp;
}

inline __Vector3 __Vector3::operator * (const D3DVECTOR& vec) const 
{
	static __Vector3 vTmp;

	vTmp.x = x * vec.x;
	vTmp.y = y * vec.y;
	vTmp.z = z * vec.z;
	return vTmp;
}

inline __Vector3 __Vector3::operator / (const D3DVECTOR& vec) const
{
	static __Vector3 vTmp;

	vTmp.x = x / vec.x;
	vTmp.y = y / vec.y;
	vTmp.z = z / vec.z;
	return vTmp;
}

inline void __Vector3::operator += (const D3DVECTOR& vec) 
{
	x += vec.x;
	y += vec.y;
	z += vec.z;
}

inline void __Vector3::operator -= (const D3DVECTOR& vec) 
{
	x -= vec.x;
	y -= vec.y;
	z -= vec.z;
}

inline void __Vector3::operator *= (const D3DVECTOR& vec) 
{
	x *= vec.x;
	y *= vec.y;
	z *= vec.z;
}

inline void __Vector3::operator /= (const D3DVECTOR& vec) 
{
	x /= vec.x;
	y /= vec.y;
	z /= vec.z;
}

inline __Vector3 __Vector3::operator + (float fDelta) const 
{ 
	static __Vector3 vTmp;

	vTmp.x = x + fDelta;
	vTmp.y = y + fDelta;
	vTmp.z = z + fDelta;
	return vTmp;
}

inline __Vector3 __Vector3::operator - (float fDelta) const 
{
	static __Vector3 vTmp;

	vTmp.x = x - fDelta;
	vTmp.y = y - fDelta;
	vTmp.z = z - fDelta;
	return vTmp;
}

inline __Vector3 __Vector3::operator * (float fDelta) const 
{
	static __Vector3 vTmp;

	vTmp.x = x * fDelta;
	vTmp.y = y * fDelta;
	vTmp.z = z * fDelta;
	return vTmp;
}

inline __Vector3 __Vector3::operator / (float fDelta) const 
{
	static __Vector3 vTmp;

	vTmp.x = x / fDelta;
	vTmp.y = y / fDelta;
	vTmp.z = z / fDelta;
	return vTmp;
}

inline void __Matrix44::Identity()
{
	_12 = _13 = _14 = _21 = _23 = _24 = _31 = _32 = _34 = _41 = _42 = _43 = 0;
	_11 = _22 = _33 = _44 = 1.0f;
}

inline void __Matrix44::RotationY(float fDelta)
{
	this->Identity();
	_11 = cosf(fDelta); _13 = -sinf(fDelta); _31 = -_13; _33 = _11;
}

bool			_IntersectTriangle(const __Vector3& vOrig, const __Vector3& vDir , const __Vector3& v0, const __Vector3& v1, const __Vector3& v2, float& fT, float& fU, float& fV, __Vector3* pVCol = NULL);
bool			_IntersectTriangle(const __Vector3& vOrig, const __Vector3& vDir, const __Vector3& v0, const __Vector3& v1, const __Vector3& v2);

inline bool _IntersectTriangle(const __Vector3& vOrig, const __Vector3& vDir,
							  const __Vector3& v0, const __Vector3& v1, const __Vector3& v2,
							  float& fT, float& fU, float& fV, __Vector3* pVCol)
{
    // Find vectors for two edges sharing vert0
    static __Vector3 vEdge1, vEdge2;
	
	vEdge1 = v1 - v0;
    vEdge2 = v2 - v0;

    // Begin calculating determinant - also used to calculate U parameter
    __Vector3 pVec;	float fDet;
	
//	By : Ecli666 ( On 2001-09-12 오전 10:39:01 )

	pVec.Cross(vEdge1, vEdge2);
	fDet = pVec.Dot(vDir);
	if ( fDet > -0.0001f )
		return FALSE;

//	~(By Ecli666 On 2001-09-12 오전 10:39:01 )

    pVec.Cross(vDir, vEdge2);

    // If determinant is near zero, ray lies in plane of triangle
    fDet = vEdge1.Dot(pVec);
    if( fDet < 0.0001f )		// 거의 0에 가까우면 삼각형 평면과 지나가는 선이 평행하다.
        return FALSE;

    // Calculate distance from vert0 to ray origin
    __Vector3 tVec = vOrig - v0;

    // Calculate U parameter and test bounds
    fU = tVec.Dot(pVec);
    if( fU < 0.0f || fU > fDet )
        return FALSE;

    // Prepare to test V parameter
    __Vector3 qVec;
    qVec.Cross(tVec, vEdge1);

    // Calculate V parameter and test bounds
    fV = vDir.Dot(qVec);
    if( fV < 0.0f || fU + fV > fDet )
        return FALSE;

    // Calculate t, scale parameters, ray intersects triangle
	fT = vEdge2.Dot(qVec);
    float fInvDet = 1.0f / fDet;
    fT *= fInvDet;
    fU *= fInvDet;
    fV *= fInvDet;

	// t가 클수록 멀리 직선과 평면과 만나는 점이 멀다.
	// t*dir + orig 를 구하면 만나는 점을 구할 수 있다.
	// u와 v의 의미는 무엇일까?
	// 추측 : v0 (0,0), v1(1,0), v2(0,1) <괄호안은 (U, V)좌표> 이런식으로 어느 점에 가깝나 나타낸 것 같음
	//

	if(pVCol) (*pVCol) = vOrig + (vDir * fT);	// 접점을 계산..

	// *t < 0 이면 뒤쪽...
	if ( fT < 0.0f )
		return FALSE;

	return TRUE;
}

inline bool _IntersectTriangle(const __Vector3& vOrig, const __Vector3& vDir, const __Vector3& v0, const __Vector3& v1, const __Vector3& v2)
{
    // Find vectors for two edges sharing vert0
    // Begin calculating determinant - also used to calculate U parameter
    static float fDet, fT, fU, fV;
    static __Vector3 vEdge1, vEdge2, tVec, pVec, qVec;
	
	vEdge1 = v1 - v0;
    vEdge2 = v2 - v0;

	
//	By : Ecli666 ( On 2001-09-12 오전 10:39:01 )

	pVec.Cross(vEdge1, vEdge2);
	fDet = pVec.Dot(vDir);
	if ( fDet > -0.0001f )
		return FALSE;

//	~(By Ecli666 On 2001-09-12 오전 10:39:01 )

    pVec.Cross(vDir, vEdge2);

    // If determinant is near zero, ray lies in plane of triangle
    fDet = vEdge1.Dot(pVec);
    if( fDet < 0.0001f )		// 거의 0에 가까우면 삼각형 평면과 지나가는 선이 평행하다.
        return FALSE;

    // Calculate distance from vert0 to ray origin
    tVec = vOrig - v0;

    // Calculate U parameter and test bounds
    fU = tVec.Dot(pVec);
    if( fU < 0.0f || fU > fDet )
        return FALSE;

    // Prepare to test V parameter
    qVec.Cross(tVec, vEdge1);

    // Calculate V parameter and test bounds
	fV = vDir.Dot(qVec);
    if( fV < 0.0f || fU + fV > fDet )
        return FALSE;

    // Calculate t, scale parameters, ray intersects triangle
    fT = vEdge2.Dot(qVec) / fDet;

	// *t < 0 이면 뒤쪽...
	if ( fT < 0.0f )
		return FALSE;

	return TRUE;
}