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
        _ObjectManager(T * const Allocation) :
            m_Allocation(Allocation)
        {

        };

        ~_ObjectManager(void)
        {
            assert(m_Refs == 0);
            delete m_Allocation;
        }
        inline T* const get(void) const { return m_Allocation; };
        inline void ref(void) { ++m_Refs; };
        inline const size_t deref(void) { return --m_Refs; };
        inline const size_t refs(void) const { return m_Refs; };
        friend class SharedRefPtr<T>;
    private:
        T* const m_Allocation = nullptr;
        size_t m_Refs = 0;
    };

    template <typename T>
    class SharedRefPtr
    {
    public:

        SharedRefPtr(
            void
        )
        {
            m_Manager = nullptr;
        }

        SharedRefPtr(
            T* const Allocation
        )
        {
            m_Manager = new _ObjectManager(std::move(Allocation));
            m_Manager->ref();
        }

        SharedRefPtr(
            const SharedRefPtr& Rhs
        )
        {
            m_Manager = Rhs.m_Manager;
            m_Manager->ref();
        }

        // SharedRefPtr(
        //     SharedRefPtr&& Rhs
        // )
        // {
        //     m_Manager = Rhs.m_Manager;
        //     Rhs.m_Manager = nullptr;
        // }

        SharedRefPtr&
        operator=(
            SharedRefPtr&& Other
        )
        /*++
          Move assignment
        --*/
        {
            m_Manager = Other.m_Manager;
            Other.m_Manager = nullptr;
            return *this;
        }

        SharedRefPtr&
        operator=(
            const SharedRefPtr& Other
        )
        /*++
          Copy assignment
        --*/
        {
            m_Manager = Other.m_Manager;
            m_Manager->ref();
            return *this;
        }

        bool
        operator==(
            const SharedRefPtr& Other
        ) const
        {
            return (m_Manager == nullptr && Other.m_Manager == nullptr) || (m_Manager->m_Allocation == Other.m_Manager->m_Allocation);
        }

        bool
        operator==(
            const void* Ptr
        ) const
        {
            return (m_Manager == nullptr) || (m_Manager->m_Allocation == Ptr);
        }

        ~SharedRefPtr(
            void
        )
        {
            if (m_Manager != nullptr &&
                m_Manager->deref() == 0)
            {
                delete m_Manager;
            }
        }

        inline T*
        get(
            void
        ) const
        {
            return m_Manager->get();
        }

        inline T*
        operator->(
            void
        ) const
        {
            return get();
        }

        inline T&
        operator*(
            void
        ) const
        {
            return *m_Manager->get();
        }

    protected:
        _ObjectManager<T>* m_Manager = nullptr;
    };


    template <typename T, class... Args>
    dispatch::SharedRefPtr<T>
    MakeSharedRefPtr(
        Args&&... x
    )
    {
        T* allocation = new T(std::forward<Args>(x)...);
        return SharedRefPtr<T>(allocation);
    }
}