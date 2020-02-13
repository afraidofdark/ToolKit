#pragma once

#include "Primative.h"
#include "Types.h"

namespace ToolKit
{
	class Material;

	namespace Editor
	{
		class Grid : public Drawable
		{
		public:
			Grid(uint size);
			void Resize(uint size);

		public:
			uint m_size; // m^2 size of the grid.
		};
	}
}