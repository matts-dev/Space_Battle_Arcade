#include "Widget3D_DClockFontTest.h"
#include "../text/DigitalClockFont.h"

namespace SA
{
	void Widget3D_DClockFontTest::postConstruct()
	{
		rawGlyph = new_sp<DigitalClockGlyph>();
		glyphShader = getDefaultGlyphShader_uniformBased();

	}

	void Widget3D_DClockFontTest::render(GameUIRenderData& renderData)
	{
		if (glyphShader)
		{
			rawGlyph->render(renderData, *glyphShader);
		}
	}

}

