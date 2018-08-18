#pragma once

/** mesh interface */
class Mesh
{
public:
	Mesh() = default;
	virtual ~Mesh() = default;
	virtual void render() = 0;
};