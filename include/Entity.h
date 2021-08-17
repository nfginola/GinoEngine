#pragma once
#include "Component.h"

#include "Graphics/Model.h"

namespace Gino
{
	// Map component type to real types with some metadata for Entity usage
	template <ComponentType>
	struct ComponentMapper;

	template <>
	struct ComponentMapper<ComponentType::TransformType> 
	{ 
		using type = Transform; 
		static constexpr uint32_t bit = ComponentType::TransformType;
		static constexpr int index = bit - 1;
	};

	template <>
	struct ComponentMapper<ComponentType::ModelType> 
	{ 
		using type = Model; 
		static constexpr uint32_t bit = ComponentType::ModelType;
		static constexpr int index = bit - 1;
	};


	class Entity
	{
	public:
		Entity();
		~Entity();

		template <ComponentType T>
		void AddComponent(Component* comp);

		template <ComponentType T> 
		auto GetComponent() const;


	private:
		std::array<Component*, 16> m_components;

		// Bitflags using ComponentType
		uint32_t m_activeComponentsBits;		

	};

	template<ComponentType T>
	inline void Entity::AddComponent(Component* comp)
	{
		// Check that the component given is the right type
		assert((comp->GetBit() & ComponentMapper<T>::bit) == ComponentMapper<T>::bit);

		// We dont add components if it already exists
		assert((m_activeComponentsBits & ComponentMapper<T>::bit) != ComponentMapper<T>::bit);

		m_components[ComponentMapper<T>::index] = comp;
		m_activeComponentsBits |= ComponentMapper<T>::bit;
	}

	template<ComponentType T>
	auto Entity::GetComponent() const
	{
		// Make sure the component exists!
		assert((m_activeComponentsBits & ComponentMapper<T>::bit) == ComponentMapper<T>::bit);
		return static_cast<ComponentMapper<T>::type*>(m_components[ComponentMapper<T>::index]);
	}
}


