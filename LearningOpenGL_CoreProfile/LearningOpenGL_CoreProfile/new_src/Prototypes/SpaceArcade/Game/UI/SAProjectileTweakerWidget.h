#pragma once

namespace SA
{
	/** Provides the ability load/save projectile classes and tweak their handles in real time 
		usage: create a ImGui window with ImGui::Begin(), call this widget's function to fill the window,
		then after calling that function end the window with ImGuid::End() */
	class ProjectileTweakerWidget
	{
	public:
		/** populate a preexisting window with this widget's contents.
			#Invariant: this method must be called between ImGui::Begin and ImGui::End();
		*/
		void renderInCurrentUIWindow();

	};

}
