#pragma once

#include <assert.h>

namespace dispatch
{
    template <typename T>
    class SharedRefPtr;

    template <typename T>
    class _ObjectManager
    {
    protected:
        _ObjectManager(T* Allocation) : m_Allocation(Allocation) {};
        ~_ObjectManager(void)
        {
            assert(m_Refs == 0);
            delete m_Allocation;
        }
        inline T* get(void) const { return m_Allocation; };
        inline void ref(void) { ++m_Refs; };
        inline const size_t deref(void) { return --m_Refs; };
        inline const size_t refs(void) const { return m_Refs; };
        friend class SharedRefPtr<T>;
    private:
        T* m_Allocation = nullptr;
        size_t m_Refs = 0;
    };

    template <typename T>
    class SharedRefPtr
    {
    public:
        template <class... Args>
        SharedRefPtr(Args&&... x){
            auto allocation = new T(std::forward<Args>(x)...);
            m_Manager = new _ObjectManager(std::move(allocation));
            m_Manager->ref();
        }

        SharedRefPtr(
            SharedRefPtr& Rhs
        )
        {
            std::cout << "Incrementing" << std::endl;
            m_Manager = Rhs.m_Manager;
            m_Manager->ref();
        }

        SharedRefPtr(
            SharedRefPtr&& Rhs
        )
        {
            m_Manager = Rhs.m_Manager;
            m_Manager = nullptr;
        }

        ~SharedRefPtr(
            void
        )
        {
            if (m_Manager->deref() == 0)
            {
                delete m_Manager;
            }
        }

        inline T*
        get(void)
        {
            return m_Manager->get();
        }

        inline T
        operator*(void)
        {
            return *m_Manager->get();
        }

    protected:
        _ObjectManager<T>* m_Manager = nullptr;
    };

}