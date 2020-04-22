#include "Widget3D_DigitalClockFontTest.h"
#include "../text/DigitalClockFont.h"

namespace SA
{
	void Widget3D_DigitalClockFontTest::postConstruct()
	{
		rawGlyph = new_sp<DigitalClockGlyph>();
		glyphShader = getDefaultGlyphShader_uniformBased();

	}

	void Widget3D_DigitalClockFontTest::render(GameUIRenderData& renderData)
	{
		if (glyphShader)
		{
			rawGlyph->render(renderData, *glyphShader);
		}
	}

}

