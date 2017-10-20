//
// Created by byter on 29.07.17.
//

#include "Renderer_impl.h"

namespace three {

OpenGLRenderer::Ptr OpenGLRenderer::make(QOpenGLContext *context, float width, float height, const OpenGLRendererOptions &options)
{
  return std::shared_ptr<OpenGLRenderer>(new gl::Renderer_impl(context, width, height));
}

namespace gl {

Renderer_impl::Renderer_impl(QOpenGLContext *context, unsigned width, unsigned height, bool premultipliedAlpha)
   : OpenGLRenderer(context), _state(this), _attributes(this), _objects(_geometries, _infoRender),
     _geometries(_attributes), _extensions(context), _capabilities(this, _extensions, _parameters ),
     _morphTargets(this), _programs(_extensions, _capabilities),
     _background(*this, _state, _geometries, _premultipliedAlpha)
{

}

void Renderer_impl::initContext()
{
  initializeOpenGLFunctions();

  _extensions.get({Extension::ARB_depth_texture, Extension::OES_texture_float,
                   Extension::OES_texture_float_linear, Extension::OES_texture_half_float,
                   Extension::OES_texture_half_float_linear, Extension::OES_standard_derivatives,
                   Extension::ANGLE_instanced_arrays});

  if (_extensions.get(Extension::OES_element_index_uint) ) {

    BufferGeometry::MaxIndex = 4294967296;
  }

  _capabilities.init();

  _state.init();
  _currentScissor = _scissor * _pixelRatio;
  _currentViewport = _viewport * _pixelRatio;
  _state.scissor(_currentScissor);
  _state.viewport(_currentViewport);

  //textures = new WebGLTextures( _gl, extensions, state, properties, capabilities, paramThreeToGL, _infoMemory );
#if 0
  background = new WebGLBackground( _this, state, geometries, _premultipliedAlpha );
  bufferRenderer = new WebGLBufferRenderer( _gl, extensions, _infoRender );
  indexedBufferRenderer = new WebGLIndexedBufferRenderer( _gl, extensions, _infoRender );
  flareRenderer = new WebGLFlareRenderer( _this, _gl, state, textures, capabilities );
  spriteRenderer = new WebGLSpriteRenderer( _this, _gl, state, textures, capabilities );

  _info.programs = programCache.programs;
#endif
}

void Renderer_impl::clear(bool color, bool depth, bool stencil)
{
  unsigned bits = 0;

  if (color) bits |= GL_COLOR_BUFFER_BIT;
  if (depth) bits |= GL_DEPTH_BUFFER_BIT;
  if (stencil) bits |= GL_STENCIL_BUFFER_BIT;

  glClear( bits );
}

void Renderer_impl::doRender(const Scene::Ptr &scene, const Camera::Ptr &camera,
                             const Renderer::Target::Ptr &renderTarget, bool forceClear)
{
  if (_isContextLost) return;

  // reset caching for this frame

  _currentGeometryProgram = 0;
  _currentMaterialId = -1;
  _currentCamera = nullptr;

  // update scene graph
  if (scene->autoUpdate()) scene->updateMatrixWorld(false);

  // update camera matrices and frustum

  if (!camera->parent()) camera->updateMatrixWorld(false);

  /*if ( vr.enabled ) {

    camera = vr.getCamera( camera );

  }*/

  _projScreenMatrix = camera->projectionMatrix() * camera->matrixWorldInverse();
  _frustum.set(_projScreenMatrix);

  _lightsArray.clear();
  _shadowsArray.clear();

  _spritesArray.clear();
  _flaresArray.clear();

  _clippingEnabled = _clipping.init(_clippingPlanes, _localClippingEnabled, camera);

  _currentRenderList = _renderLists.get(scene, camera);
  _currentRenderList->init();

  projectObject(scene, camera, _sortObjects);

  if (_sortObjects) {
    _currentRenderList->sort();
  }

  //
  if (_clippingEnabled) _clipping.beginShadows();

  _shadowMap->render(_shadowsArray, scene, camera);

  _lights.setup(_lightsArray, camera);

  if (_clippingEnabled) _clipping.endShadows();

  //

  _infoRender.frame++;
  _infoRender.calls = 0;
  _infoRender.vertices = 0;
  _infoRender.faces = 0;
  _infoRender.points = 0;

  setRenderTarget(renderTarget);
#if 0
  //
  _background.render(currentRenderList, scene, camera, forceClear);

  // render scene
  var opaqueObjects = currentRenderList.opaque;
  var transparentObjects = currentRenderList.transparent;

  if (scene.overrideMaterial) {
    var overrideMaterial = scene.overrideMaterial;

    if (opaqueObjects.length) renderObjects(opaqueObjects, scene, camera, overrideMaterial);
    if (transparentObjects.length) renderObjects(transparentObjects, scene, camera, overrideMaterial);
  }
  else {
    // opaque pass (front-to-back order)
    if (opaqueObjects.length) renderObjects(opaqueObjects, scene, camera);

    // transparent pass (back-to-front order)
    if (transparentObjects.length) renderObjects(transparentObjects, scene, camera);
  }

  // custom renderers

  spriteRenderer.render(spritesArray, scene, camera);
  flareRenderer.render(flaresArray, scene, camera, _currentViewport);

  // Generate mipmap if we're using any kind of mipmap filtering

  if (renderTarget) {
    _textures.updateRenderTargetMipmap(renderTarget);
  }

  // Ensure depth buffer writing is enabled so it can be cleared on next render
  _state.depthBuffer.setTest(true);
  _state.depthBuffer.setMask(true);
  _state.colorBuffer.setMask(true);
#endif
  /*if (vr.enabled) {
    vr.submitFrame();
  }*/
  // _gl.finish();
}

unsigned Renderer_impl::allocTextureUnit()
{
  unsigned textureUnit = _usedTextureUnits;

  if(textureUnit >= _capabilities.maxTextures ) {
    throw std::logic_error("max texture units exceeded");
  }

  _usedTextureUnits += 1;

  return textureUnit;
}

void setTexture2D(Texture::Ptr texture, unsigned slot )
{
  //_textures.setTexture2D( texture, slot );
}

#if 0
Renderer_impl& Renderer_impl::setRenderTarget( renderTarget ) {

  _currentRenderTarget = renderTarget;

  if ( renderTarget && properties.get( renderTarget ).__webglFramebuffer === undefined ) {

    textures.setupRenderTarget( renderTarget );

  }

  var framebuffer = null;
  var isCube = false;

  if ( renderTarget ) {

    var __webglFramebuffer = properties.get( renderTarget ).__webglFramebuffer;

    if ( renderTarget.isWebGLRenderTargetCube ) {

      framebuffer = __webglFramebuffer[ renderTarget.activeCubeFace ];
      isCube = true;

    } else {

      framebuffer = __webglFramebuffer;

    }

    _currentViewport.copy( renderTarget.viewport );
    _currentScissor.copy( renderTarget.scissor );
    _currentScissorTest = renderTarget.scissorTest;

  } else {

    _currentViewport.copy( _viewport ).multiplyScalar( _pixelRatio );
    _currentScissor.copy( _scissor ).multiplyScalar( _pixelRatio );
    _currentScissorTest = _scissorTest;

  }

  if ( _currentFramebuffer !== framebuffer ) {

    _gl.bindFramebuffer( _gl.FRAMEBUFFER, framebuffer );
    _currentFramebuffer = framebuffer;

  }

  state.viewport( _currentViewport );
  state.scissor( _currentScissor );
  state.setScissorTest( _currentScissorTest );

  if ( isCube ) {

    var textureProperties = properties.get( renderTarget.texture );
    _gl.framebufferTexture2D( _gl.FRAMEBUFFER, _gl.COLOR_ATTACHMENT0, _gl.TEXTURE_CUBE_MAP_POSITIVE_X + renderTarget.activeCubeFace, textureProperties.__webglTexture, renderTarget.activeMipMapLevel );

  }

}

void Renderer_impl::projectObject(Object3D::Ptr object, Camera::Ptr camera, bool sortObjects )
{
  if (!object->visible()) return;

  bool visible = object->layers().test(camera->layers());

  if ( visible ) {

    if ( object.isLight ) {

      lightsArray.push( object );

      if ( object.castShadow ) {

        shadowsArray.push( object );

      }

    } else if ( object.isSprite ) {

      if ( ! object.frustumCulled || _frustum.intersectsSprite( object ) ) {

        spritesArray.push( object );

      }

    } else if ( object.isLensFlare ) {

      flaresArray.push( object );

    } else if ( object.isImmediateRenderObject ) {

      if ( sortObjects ) {

        _vector3.setFromMatrixPosition( object.matrixWorld )
           .applyMatrix4( _projScreenMatrix );

      }

      currentRenderList.push( object, null, object.material, _vector3.z, null );

    } else if ( object.isMesh || object.isLine || object.isPoints ) {

      if ( object.isSkinnedMesh ) {

        object.skeleton.update();

      }

      if ( ! object.frustumCulled || _frustum.intersectsObject( object ) ) {

        if ( sortObjects ) {

          _vector3.setFromMatrixPosition( object.matrixWorld )
             .applyMatrix4( _projScreenMatrix );

        }

        var geometry = objects.update( object );
        var material = object.material;

        if ( Array.isArray( material ) ) {

          var groups = geometry.groups;

          for ( var i = 0, l = groups.length; i < l; i ++ ) {

            var group = groups[ i ];
            var groupMaterial = material[ group.materialIndex ];

            if ( groupMaterial && groupMaterial.visible ) {

              currentRenderList.push( object, geometry, groupMaterial, _vector3.z, group );

            }

          }

        } else if ( material.visible ) {

          currentRenderList.push( object, geometry, material, _vector3.z, null );

        }

      }

    }

  }

  var children = object.children;

  for ( var i = 0, l = children.length; i < l; i ++ ) {

    projectObject( children[ i ], camera, sortObjects );

  }
}
#endif

void Renderer_impl::renderBufferDirect(Camera::Ptr camera,
                                       const Fog *fog,
                                       Geometry::Ptr geometry,
                                       Material::Ptr material,
                                       Object3D::Ptr object,
                                       const Group &group)
{
#if 0
  state.setMaterial( material );

  var program = setProgram( camera, fog, material, object );
  var geometryProgram = geometry.id + '_' + program.id + '_' + ( material.wireframe === true );

  var updateBuffers = false;

  if ( geometryProgram !== _currentGeometryProgram ) {

    _currentGeometryProgram = geometryProgram;
    updateBuffers = true;

  }

  if ( object.morphTargetInfluences ) {

    morphtargets.update( object, geometry, material, program );

    updateBuffers = true;

  }

  //

  var index = geometry.index;
  var position = geometry.attributes.position;
  var rangeFactor = 1;

  if ( material.wireframe === true ) {

    index = geometries.getWireframeAttribute( geometry );
    rangeFactor = 2;

  }

  var attribute;
  var renderer = bufferRenderer;

  if ( index !== null ) {

    attribute = attributes.get( index );

    renderer = indexedBufferRenderer;
    renderer.setIndex( attribute );

  }

  if ( updateBuffers ) {

    setupVertexAttributes( material, program, geometry );

    if ( index !== null ) {

      _gl.bindBuffer( _gl.ELEMENT_ARRAY_BUFFER, attribute.buffer );

    }

  }

  //

  var dataCount = 0;

  if ( index !== null ) {

    dataCount = index.count;

  } else if ( position !== undefined ) {

    dataCount = position.count;

  }

  var rangeStart = geometry.drawRange.start * rangeFactor;
  var rangeCount = geometry.drawRange.count * rangeFactor;

  var groupStart = group !== null ? group.start * rangeFactor : 0;
  var groupCount = group !== null ? group.count * rangeFactor : Infinity;

  var drawStart = Math.max( rangeStart, groupStart );
  var drawEnd = Math.min( dataCount, rangeStart + rangeCount, groupStart + groupCount ) - 1;

  var drawCount = Math.max( 0, drawEnd - drawStart + 1 );

  if ( drawCount === 0 ) return;

  //

  if ( object.isMesh ) {

    if ( material.wireframe === true ) {

      state.setLineWidth( material.wireframeLinewidth * getTargetPixelRatio() );
      renderer.setMode( _gl.LINES );

    } else {

      switch ( object.drawMode ) {

        case TrianglesDrawMode:
          renderer.setMode( _gl.TRIANGLES );
          break;

        case TriangleStripDrawMode:
          renderer.setMode( _gl.TRIANGLE_STRIP );
          break;

        case TriangleFanDrawMode:
          renderer.setMode( _gl.TRIANGLE_FAN );
          break;

      }

    }


  } else if ( object.isLine ) {

    var lineWidth = material.linewidth;

    if ( lineWidth === undefined ) lineWidth = 1; // Not using Line*Material

    state.setLineWidth( lineWidth * getTargetPixelRatio() );

    if ( object.isLineSegments ) {

      renderer.setMode( _gl.LINES );

    } else if ( object.isLineLoop ) {

      renderer.setMode( _gl.LINE_LOOP );

    } else {

      renderer.setMode( _gl.LINE_STRIP );

    }

  } else if ( object.isPoints ) {

    renderer.setMode( _gl.POINTS );

  }

  if ( geometry && geometry.isInstancedBufferGeometry ) {

    if ( geometry.maxInstancedCount > 0 ) {

      renderer.renderInstances( geometry, drawStart, drawCount );

    }

  } else {

    renderer.render( drawStart, drawCount );

  }
#endif
}

}
}