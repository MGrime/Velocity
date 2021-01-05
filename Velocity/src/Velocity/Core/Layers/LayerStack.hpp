#pragma once

#include "Layer.hpp"

#include <vector>

namespace Velocity
{
	class LayerStack
	{
	public:
		using LayerIterator = std::vector<Layer*>::iterator;
		using LayerConstIterator = std::vector<Layer*>::const_iterator;

		LayerStack() = default;
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		LayerIterator begin() { return m_Layers.begin(); }
		LayerIterator end() { return m_Layers.end(); }
	
	private:
		std::vector<Layer*> m_Layers;
		uint32_t			m_LayerInsertIndex = 0;
		
	};
}
