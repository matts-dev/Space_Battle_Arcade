#pragma once


namespace SA
{
	/////////////////////////////////////////////////////////////////////////////////////
	// interface to register object to be ticked.
	/////////////////////////////////////////////////////////////////////////////////////
	class ITickable
	{
		friend class TimeManager;
	protected:
		/*  Ticks the current object with the dilated delta time seconds.
				@note: protected access, not private, to allow sub classes to call their super's tick.
				@return return true to continue being ticked, return false to be removed from ticker.*/
		virtual bool tick(float dt_sec) = 0;
	};

	//#TODO remove this class and replace inheritance with ITickable; may require work because the time manager is not the one doing the ticks
	//#TODO or rename to "TickMixin" if it isn't appropriate for all methods to be wired up to a time manager's tick.
	class Tickable
	{
		/** Parameter is the delta time in seconds; ie the time between the current and last frame */
		virtual void tick(float dt_sec) {};
	};
}