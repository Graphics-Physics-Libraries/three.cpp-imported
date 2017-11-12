//
// Created by byter on 30.09.17.
//

#ifndef THREE_QT_UNIFORMSLIB_H
#define THREE_QT_UNIFORMSLIB_H

#include <textures/DataTexture.h>
#include <textures/CubeTexture.h>
#include <core/Color.h>
#include <math/Matrix3.h>
#include <math/Vector3.h>

namespace three {
namespace gl {

extern DataTexture::Ptr LTC_MAT_TEXTURE;
extern DataTexture::Ptr LTC_MAG_TEXTURE;

/**
 * Uniforms library for RectAreaLight shared webgl shaders
 * @author abelnation
 *
 * NOTE: This is a temporary location for the BRDF approximation texture data
 *       based off of Eric Heitz's work (see citation below).  BRDF data for
 *       RectAreaLight is currently approximated using a precomputed texture
 *       of roughly 80kb in size.  The hope is to find a better way to include
 *       the large texture data before including the full RectAreaLight implementation
 *       in the main build files.
 *
 * TODO: figure out a way to compress the LTC BRDF data
 * TODO: validate the RectAreaLight BRDF and shaders
 */
void initRectAreaLight();

enum class UniformName
{
  cube,
  equirect,
  flip,
  opacity,
  diffuse,
  emissive,
  specular,
  projectionMatrix,
  viewMatrix,
  modelViewMatrix,
  normalMatrix,
  modelMatrix,
  logDepthBufFC,
  boneMatrices,
  bindMatrix,
  bindMatrixInverse,
  toneMappingExposure,
  toneMappingWhitePoint,
  cameraPosition,
  map,
  uvTransform,
  alphaMap,
  specularMap,
  envMap,
  flipEnvMap,
  reflectivity,
  refractionRatio,
  aoMap,
  aoMapIntensity,
  lightMap,
  lightMapIntensity,
  emissiveMap,
  bumpMap,
  bumpScale,
  normalMap,
  normalScale,
  displacementMap,
  displacementScale,
  displacementBias,
  roughnessMap,
  metalnessMap,
  gradientMap,
  roughness,
  metalness,
  envMapIntensity,
  fogDensity,
  fogNear,
  fogFar,
  fogColor,
  ambientLightColor,
  direction,
  color,
  shadow,
  shadowBias,
  shadowRadius,
  shadowMapSize,
  size,
  scale,
  dashSize,
  totalSize,
  referencePosition,
  nearDistance,
  farDistance,
  clippingPlanes,
  directionalLights,
  spotLights,
  rectAreaLights,
  pointLights,
  hemisphereLights,
  directionalShadowMap,
  directionalShadowMatrix,
  spotShadowMap,
  spotShadowMatrix,
  pointShadowMap,
  pointShadowMatrix
};

enum class UniformsID {
  common,
  envmap,
  aomap,
  lightmap,
  specularmap,
  emissivemap,
  bumpmap,
  normalmap,
  displacementmap,
  roughnessmap,
  metalnessmap,
  gradientmap,
  fog,
  lights,
  points
};

class Uniform;

namespace uniformslib {

struct UniformValue
{
  const UniformName id;

  bool needsUpdate = false;

  virtual ~UniformValue() {}

  using Ptr = std::shared_ptr<UniformValue>;
  explicit UniformValue(UniformName nm) : id(nm) {}

  template <typename T>
  UniformValue &operator = (T t);

  virtual Ptr clone() const = 0;
  virtual void setValue(std::shared_ptr<Uniform> uniform) = 0;
};

template<typename T>
struct UniformValueT;

#define UNIFORM_VALUE_T(Cls) \
template<> struct UniformValueT<Cls> : public UniformValue \
{ \
  Cls value; \
  explicit UniformValueT(UniformName nm, const Cls &value) : UniformValue(nm), value(value) {} \
  UniformValueT &operator = (const Cls &value) { \
    this->value = value; return *this; \
  } \
  Ptr clone() const override { \
    return Ptr(new UniformValueT(id, value)); \
  } \
  void setValue(std::shared_ptr<Uniform> uniform) override; \
}; \
inline void UniformValueT<Cls>::setValue(std::shared_ptr<Uniform> uniform)

UNIFORM_VALUE_T(math::Matrix3) {
}
UNIFORM_VALUE_T(float) {
}
UNIFORM_VALUE_T(Color) {
}
UNIFORM_VALUE_T(int) {
}
UNIFORM_VALUE_T(Texture::Ptr) {
}
UNIFORM_VALUE_T(CubeTexture::Ptr) {
}
UNIFORM_VALUE_T(math::Vector2) {
}
UNIFORM_VALUE_T(math::Vector3) {
}
UNIFORM_VALUE_T(std::vector<float>) {
}
UNIFORM_VALUE_T(math::Matrix4) {
}
UNIFORM_VALUE_T(std::vector<Texture::Ptr>) {
}
UNIFORM_VALUE_T(std::vector<math::Matrix4>) {
}

class LibUniformValues
{
  std::unordered_map<UniformName, UniformValue::Ptr> values;

public:
  LibUniformValues(std::initializer_list<UniformValue::Ptr> vals) {
    for(auto it = std::begin(vals); it != std::end(vals); it++) {
      const UniformValue::Ptr val = *it;
      values[val->id] = val;
    }
  }

  std::unordered_map<UniformName, UniformValue::Ptr> cloneValues() const
  {
    std::unordered_map<UniformName, UniformValue::Ptr> cloned;

    for(auto &entry : values) {
      cloned[entry.first] = entry.second->clone();
    }
    return cloned;
  };

  UniformValue::Ptr operator[](UniformName name) const {
    return values.at(name);
  }

  LibUniformValues &operator +=(const UniformValue::Ptr val)
  {
    values.insert({val->id, val});
    return *this;
  }

  LibUniformValues &operator +=(const LibUniformValues &vals)
  {
    values.insert(vals.values.begin(), vals.values.end());
    return *this;
  }

  LibUniformValues merge(const LibUniformValues &values)
  {
    LibUniformValues merged(values);
    merged.values.insert(values.values.begin(), values.values.end());
    return merged;
  }
};

template <typename T>
UniformValue &UniformValue::operator = (T t) {
  UniformValueT<T> &ut = dynamic_cast<UniformValueT<T> &>(*this);
  ut = t;
  return *this;
}

template<typename T>
inline UniformValue::Ptr value(UniformName id, T value) {
  return UniformValue::Ptr(new UniformValueT<T>(id, value));
}

struct UniformValueDelegate
{
  UniformValue::Ptr value;

  template<typename T> UniformValueDelegate(UniformName id, T t)
  {
    value = UniformValue::Ptr(new UniformValueT<T>(id, t));
  }

  operator UniformValue::Ptr () const {return value;}
};

struct UniformValuesDelegate
{
  LibUniformValues &values;

  explicit UniformValuesDelegate(LibUniformValues &values) : values(values) {}

  operator const LibUniformValues &() const {return values;}

  UniformValuesDelegate &merge(UniformsID id, std::initializer_list<UniformValueDelegate> add);
};

LibUniformValues &get(UniformsID id);

UniformValuesDelegate merged(std::initializer_list<UniformsID> id);

}

}
}
#endif //THREE_QT_UNIFORMSLIB_H
