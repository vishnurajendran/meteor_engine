#ifndef SUBSYSTEM_REGISTRY_H
#define SUBSYSTEM_REGISTRY_H

#include <unordered_map>
#include <typeindex>

#include "core/object/object.h"
#include "subsystem_interface.h"

class MEngineSubsystemRegistry : public MObject
{
    DEFINE_OBJECT_SUBCLASS(MEngineSubsystemRegistry)

public:
    static void init();
    static void cleanup();

    template<typename TInterface, typename TConcrete = TInterface, typename... Args>
    static TInterface* registerSubsystem(bool autoInit=true, Args&&... args)
    {
        static_assert(std::is_base_of_v<IEngineSubSystem, TInterface>,
            "Interface must derive from IEngineSubSystem");
        static_assert(std::is_base_of_v<TInterface, TConcrete>,
            "Concrete class must derive from Interface");

        const std::type_index type = typeid(TInterface);

        auto it = subSystems.find(type);
        if (it != subSystems.end())
        {
            return dynamic_cast<TInterface*>(it->second);
        }

        // Constructs the concrete class but stores it under the interface's type key
        TConcrete* inst = new TConcrete(std::forward<Args>(args)...);

        if (autoInit)
            inst->init();

        subSystems[type] = inst;

        return inst;
    }

    template<typename T>
    static T* getSubsystem()
    {
        static_assert(std::is_base_of_v<IEngineSubSystem, T>,
            "T must derive from IEngineSubSystem");

        auto it = subSystems.find(typeid(T));

        if (it == subSystems.end())
            return nullptr;

        return dynamic_cast<T*>(it->second);
    }

private:
    static std::unordered_map<std::type_index, IEngineSubSystem*> subSystems;
};

#endif