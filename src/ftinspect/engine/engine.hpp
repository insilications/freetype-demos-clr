// engine.hpp

// Copyright (C) 2016-2022 by Werner Lemberg.


#pragma once

#include "fontfilemanager.hpp"
#include "paletteinfo.hpp"
#include "fontinfo.hpp"
#include "mmgx.hpp"
#include "rendering.hpp"
#include "charmap.hpp"

#include <vector>
#include <memory>
#include <utility>
#include <QString>
#include <QMap>

#include <ft2build.h>
#include <freetype/freetype.h>
#include <freetype/ftoutln.h>
#include <freetype/ftcache.h>
#include <freetype/ftlcdfil.h>
#include <freetype/ftcolor.h>


// This structure maps the (font, face, instance) index triplet to abstract
// IDs (generated by a running number stored in MainGUI's `faceCounter'
// member).
//
// Qt's `QMap' class needs an implementation of the `<' operator.

struct FaceID
{
  int fontIndex;
  long faceIndex;
  int namedInstanceIndex;

  FaceID();
  FaceID(int fontIndex,
         long faceIndex,
         int namedInstanceIndex);
  bool operator<(const FaceID& other) const;
};

// FreeType specific data.

class Engine
{
public:
  //////// Nested definitions (forward decl)
  enum FontType : int;

  struct EngineDefaultValues
  {
    int cffHintingEngineDefault;
    int cffHintingEngineOther;

    int ttInterpreterVersionDefault;
    int ttInterpreterVersionOther;
    int ttInterpreterVersionOther1;
  };

  //////// Ctors & Dtors

  Engine();
  ~Engine();

  // Disable copying
  Engine(const Engine& other) = delete;
  Engine& operator=(const Engine& other) = delete;

  //////// Actions

  int loadFont(int fontIndex,
               long faceIndex,
               int namedInstanceIndex); // return number of glyphs
  FT_Glyph loadGlyph(int glyphIndex);
  int loadGlyphIntoSlotWithoutCache(int glyphIndex, bool noScale = false);

  // Sometimes the engine is already updated, and we want to be faster
  FT_Glyph loadGlyphWithoutUpdate(int glyphIndex,
                                  FTC_Node* outNode = NULL,
                                  bool forceRender = false);

  // reload current triplet, but with updated settings, useful for updating
  // `ftSize_` and `ftFallbackFace_` only - more convenient than `loadFont`
  void reloadFont();
  void loadPalette();

  void openFonts(QStringList const& fontFileNames);
  void removeFont(int fontIndex, bool closeFile = true);
  
  void update();
  void resetCache();
  void loadDefaults();

  //////// Getters

  FT_Library ftLibrary() const { return library_; }
  FTC_Manager cacheManager() { return cacheManager_; }
  FTC_ImageCache imageCacheManager() { return imageCache_; }
  FontFileManager& fontFileManager() { return fontFileManager_; }
  EngineDefaultValues& engineDefaults() { return engineDefaults_; }
  RenderingEngine* renderingEngine() { return renderingEngine_.get(); }
  QString dynamicLibraryVersion();

  int numberOfOpenedFonts();

  // (for current fonts)
  int currentFontIndex() { return curFontIndex_; }
  FT_Face currentFallbackFtFace() { return ftFallbackFace_; }
  FT_Size currentFtSize() { return ftSize_; }
  FT_Size_Metrics const& currentFontMetrics();
  FT_GlyphSlot currentFaceSlot();

  bool renderReady(); // Can we render bitmaps? (implys `fontValid`)
  bool fontValid(); // Is the current font valid (valid font may be unavailable
                    // to render, such as non-scalable font with invalid sizes)

  int currentFontType() const { return fontType_; }
  const QString& currentFamilyName() { return curFamilyName_; }
  const QString& currentStyleName() { return curStyleName_; }
  int currentFontNumberOfGlyphs() { return curNumGlyphs_; }
  
  std::vector<PaletteInfo>& currentFontPalettes() { return curPaletteInfos_; }
  FT_Color* currentPalette() { return palette_; }
  FT_Palette_Data& currentFontPaletteData() { return paletteData_; }
  MMGXState currentFontMMGXState() { return curMMGXState_; }
  std::vector<MMGXAxisInfo>& currentFontMMGXAxes() { return curMMGXAxes_; }
  std::vector<SFNTName>& currentFontSFNTNames() { return curSFNTNames_; }
  std::vector<CharMapInfo>& currentFontCharMaps() { return curCharMaps_; }

  QString glyphName(int glyphIndex);
  long numberOfFaces(int fontIndex);
  int numberOfNamedInstances(int fontIndex,
                             long faceIndex);
  QString namedInstanceName(int fontIndex, long faceIndex, int index);

  bool currentFontTricky();
  bool currentFontBitmapOnly();
  bool currentFontHasEmbeddedBitmap();
  bool currentFontHasColorLayers();
  bool currentFontHasGlyphName();
  
  std::vector<int> currentFontFixedSizes();
  bool currentFontPSInfo(PS_FontInfoRec& outInfo);
  bool currentFontPSPrivateInfo(PS_PrivateRec& outInfo);
  std::vector<SFNTTableInfo>& currentFontSFNTTableInfo();

  int currentFontFirstUnicodeCharMap();
  // Note: the current font face must be properly set
  unsigned glyphIndexFromCharCode(int code, int charMapIndex);
  FT_Pos currentFontTrackingKerning(int degree);
  FT_Vector currentFontKerning(int glyphIndex, int prevIndex);
  std::pair<int, int> currentSizeAscDescPx();

  // (settings)
  int dpi() { return dpi_; }
  double pointSize() { return pointSize_; }
  FTC_ImageType imageType() { return &imageType_; }
  bool antiAliasingEnabled() { return antiAliasingEnabled_; }
  bool doHinting() { return doHinting_; }
  bool embeddedBitmapEnabled() { return embeddedBitmap_; }
  bool lcdUsingSubPixelPositioning() { return lcdSubPixelPositioning_; }
  bool useColorLayer() { return useColorLayer_; }
  int paletteIndex() { return paletteIndex_; }
  FT_Render_Mode
  renderMode()
  {
    return static_cast<FT_Render_Mode>(renderMode_);
  }


  //////// Setters (direct or indirect)

  void setDPI(int d) { dpi_ = d; }
  void setSizeByPixel(double pixelSize);
  void setSizeByPoint(double pointSize);
  void setHinting(bool hinting) { doHinting_ = hinting; }
  void setAutoHinting(bool autoHinting) { doAutoHinting_ = autoHinting; }
  void setHorizontalHinting(bool horHinting)
  {
    doHorizontalHinting_ = horHinting;
  }
  void setVerticalHinting(bool verticalHinting)
  {
    doVerticalHinting_ = verticalHinting;
  }
  void setBlueZoneHinting(bool blueZoneHinting)
  {
    doBlueZoneHinting_ = blueZoneHinting;
  }
  void setShowSegments(bool showSegments) { showSegments_ = showSegments; }
  void setAntiAliasingTarget(int target) { antiAliasingTarget_ = target; }
  void setRenderMode(int mode) { renderMode_ = mode; }
  void setAntiAliasingEnabled(bool enabled) { antiAliasingEnabled_ = enabled; }
  void setEmbeddedBitmapEnabled(bool enabled) { embeddedBitmap_ = enabled; }
  void setUseColorLayer(bool colorLayer) { useColorLayer_ = colorLayer; }
  void setPaletteIndex(int index) { paletteIndex_ = index; }
  void setLCDSubPixelPositioning(bool sp) { lcdSubPixelPositioning_ = sp; }
  
  // (settings without backing fields)
  // Note: These 3 functions now takes actual mode/version from FreeType,
  // instead of values from enum in MainGUI!
  void setLcdFilter(FT_LcdFilter filter);
  void setCFFHintingMode(int mode);
  void setTTInterpreterVersion(int version);

  void setStemDarkening(bool darkening);
  void applyMMGXDesignCoords(FT_Fixed* coords, size_t count);

  //////// Misc

  friend FT_Error faceRequester(FTC_FaceID,
                                FT_Library,
                                FT_Pointer,
                                FT_Face*);

private:
  using FTC_IDType = uintptr_t;
  FTC_IDType faceCounter_; // a running number used to initialize `faceIDMap'
  QMap<FaceID, FTC_IDType> faceIDMap_;

  FontFileManager fontFileManager_;

  // font info
  int curFontIndex_ = -1;
  int fontType_;
  QString curFamilyName_;
  QString curStyleName_;
  int curNumGlyphs_ = -1;
  std::vector<CharMapInfo> curCharMaps_;
  std::vector<PaletteInfo> curPaletteInfos_;

  bool curSFNTTablesValid_ = false;
  std::vector<SFNTTableInfo> curSFNTTables_;
  MMGXState curMMGXState_ = MMGXState::NoMMGX;
  std::vector<MMGXAxisInfo> curMMGXAxes_;
  std::vector<SFNTName> curSFNTNames_;

  // basic objects
  FT_Library library_;
  FTC_Manager cacheManager_;
  FTC_ImageCache imageCache_;
  FTC_SBitCache sbitsCache_;
  FTC_CMapCache cmapCache_;
  EngineDefaultValues engineDefaults_;

  // settings
  FTC_ScalerRec scaler_ = {};
  FTC_ImageTypeRec imageType_; // for `loadGlyphWithoutUpdate`
  // Sometimes the font may be valid (i.e. a face object can be retrieved), but
  // the size may be invalid (e.g. non-scalable fonts).
  // Therefore, we use a fallback face for all non-rendering work.
  FT_Face ftFallbackFace_; // Never perform rendering or write to this!
  FT_Size ftSize_;
  FT_Palette_Data paletteData_ = {};
  FT_Color* palette_ = NULL;

  bool antiAliasingEnabled_ = true;
  bool usingPixelSize_ = false;
  double pointSize_;
  double pixelSize_;
  unsigned int dpi_;

  bool doHinting_;
  bool doAutoHinting_;
  bool doHorizontalHinting_;
  bool doVerticalHinting_;
  bool doBlueZoneHinting_;
  bool showSegments_;
  bool embeddedBitmap_;
  bool useColorLayer_;
  int paletteIndex_ = -1;
  int antiAliasingTarget_;
  bool lcdSubPixelPositioning_;
  int renderMode_;

  unsigned long loadFlags_;

  std::unique_ptr<RenderingEngine> renderingEngine_;

  void queryEngine();
  void loadPaletteInfos();

  // Safe to put the impl to the cpp.
  template <class Func>
  void withFace(FaceID id, Func func);

public:

  /// Actual definition

  // XXX cover all available modules
  enum FontType : int
  {
    FontType_CFF,
    FontType_TrueType,
    FontType_Other
  };
};


// end of engine.hpp
