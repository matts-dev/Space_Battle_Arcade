#pragma once


namespace SA
{
	class Tickable
	{
		/** Paramter is the delta time in seconds; ie the time between the current and last frame */
		virtual void tick(float dt_sec) {};
	};
}