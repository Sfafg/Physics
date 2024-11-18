#pragma once
#include <vector>
#include <iostream>
#include <set>
#include <tuple>
#include <concepts>
#include <cassert>

namespace ECS
{
#define ENTITY(...) struct ECS::Entity : ECS::EntityTemplate<__VA_ARGS__> {Entity(uint32_t index = -1) : ECS::EntityTemplate<__VA_ARGS__>(index){}}

    template <typename TComponent>
    class Component;

    template <typename TComponent>
    concept ComponentDerived = std::is_base_of_v<Component<TComponent>, TComponent>;

    template<ComponentDerived... TComponents>
    struct ComponentTable
    {
        ComponentTable();
    private:

        template <ComponentDerived TComponent>
        uint64_t GetIndex() const;

        template <ComponentDerived TComponent>
        void SetIndex(uint64_t index);

        template <ComponentDerived TComponent>
        void SetEmpty();

        template <ComponentDerived TComponent>
        bool IsEmpty();

        template <ComponentDerived TComponent>
        struct ComponentIndex { TComponent::TIndex index; };

        std::tuple<ComponentIndex<TComponents>...> table;

        template<ComponentDerived... UComponents>
        friend class EntityTemplate;

        template <ComponentDerived TComponent>
        friend class System;
    };

#define REGISTER_SYSTEMS(componentType, ...)template <> struct ECS::SystemsCollection<componentType>{using Systems = std::tuple<__VA_ARGS__>;}

    template<ComponentDerived TComponent>
    struct SystemsCollection
    {
        using Systems = std::tuple<>;
    };

    class Entity;
    template<ComponentDerived... TComponents>
    struct EntityTemplate
    {
        EntityTemplate(uint32_t index = -1);
        EntityTemplate(EntityTemplate&& other) noexcept;
        EntityTemplate(const EntityTemplate& other);

        EntityTemplate& operator=(EntityTemplate&& other) noexcept;
        EntityTemplate& operator=(const EntityTemplate& other);

        template <typename TEntity = Entity, ComponentDerived... UComponents>
        static TEntity AddEntity(UComponents&&... components);

        template <ComponentDerived TComponent>
        TComponent& AddComponent(TComponent&& component);

        template <ComponentDerived TComponent>
        bool HasComponent() const;

        template <ComponentDerived TComponent>
        TComponent& GetComponent()const;

        template <ComponentDerived TComponent>
        EntityTemplate& DestroyComponent();

        void Destroy();

    protected:
        ~EntityTemplate();

    private:
        uint32_t index;

        static std::set<EntityTemplate*> entityReferances;
        static std::vector<ComponentTable<TComponents...>> entities;

        template <typename TComponent>
        friend class Component;
        template <ComponentDerived TComponent>
        friend class System;
    };

    template <typename TComponent>
    class Component
    {
    public:
        using TIndex = uint32_t;
        using Parent = Component<TComponent>;
        Component();

        template<typename TEntity = Entity>
        TEntity GetEntity() const;

        template <ComponentDerived UComponent, typename TEntity = Entity>
        bool HasComponent() const;

        template<ComponentDerived UComponent, typename TEntity = Entity>
        UComponent& GetComponent() const;

        template<ComponentDerived UComponent, typename TEntity = Entity>
        UComponent& AddComponent(UComponent&& component);

        template <ComponentDerived UComponent, typename TEntity = Entity>
        void DestroyComponent();

        template<typename TEntity = Entity>
        void Destroy();

    private:
        uint32_t entityIndex;

        template<ComponentDerived... TComponents>
        friend class EntityTemplate;
        template<ComponentDerived UComponent>
        friend class System;
    };

    template <ComponentDerived TComponent>
    class System
    {
    public:
        static uint64_t AddComponent(TComponent&& component);
        static bool IsValid(uint64_t index);
        static TComponent& GetComponent(uint64_t index);
        template <typename TEntity>
        static void DestroyComponent(uint64_t index);

    protected:
        static std::vector<TComponent> components;
        template<ComponentDerived... TComponents>
        friend class EntityTemplate;
    };
}