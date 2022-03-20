#pragma once

#include <assert.h>

namespace dispatch
{
    template <typename T, bool Bound=false>
    class SharedRefPtr;

    class DispatcherBase;
    extern thread_local DispatcherBase* ThreadQueue;

    template <typename T, bool Bound>
    class _ObjectManager
    {
    protected:
        _ObjectManager(T * const Allocation) :
            m_Allocation(Allocation),
            m_BoundDispatcher((void*)ThreadQueue)
        {
            if(Bound)
            {
                assert(ThreadQueue != nullptr);
            }
        };

        ~_ObjectManager(void)
        {
            assert(!Bound || m_BoundDispatcher == ThreadQueue);
            assert(m_Refs == 0);
            delete m_Allocation;
        }
        inline T* const get(void) const { assert(!Bound || m_BoundDispatcher == ThreadQueue); return m_Allocation; };
        inline void ref(void) { assert(!Bound || m_BoundDispatcher == ThreadQueue); ++m_Refs; };
        inline const size_t deref(void) { assert(!Bound || m_BoundDispatcher == ThreadQueue); return --m_Refs; };
        inline const size_t refs(void) const { assert(!Bound || m_BoundDispatcher == ThreadQueue); return m_Refs; };
        friend class SharedRefPtr<T,Bound>;
    private:
        T* const m_Allocation = nullptr;
        size_t m_Refs = 0;
        void* m_BoundDispatcher = nullptr;
    };

    template <typename T, bool Bound>
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
            m_Manager = new _ObjectManager<T,Bound>(std::move(Allocation));
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

        bool
        operator!=(
            const SharedRefPtr& Other
        ) const
        {
            return !(this->operator==(Other));
        }

        bool
        operator!=(
            const void* Ptr
        ) const
        {
            return !(this->operator==(Ptr));
        }

        bool
        operator!(
            void
        ) const
        {
            return m_Manager == nullptr || m_Manager->m_Allocation == nullptr;
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
        _ObjectManager<T,Bound>* m_Manager = nullptr;
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

    template <typename T>
    using BoundRefPtr = dispatch::SharedRefPtr<T,true>;

    template <typename T, class... Args>
    dispatch::BoundRefPtr<T>
    MakeBoundRefPtr(
        Args&&... x
    )
    {
        T* allocation = new T(std::forward<Args>(x)...);
        return BoundRefPtr<T>(allocation);
    }

#define STDPTR

#ifdef STDPTR
#define MakeShared std::make_shared
#define SharedPtr std::shared_ptr
#define MakeBound std::make_shared
#define BoundPtr std::shared_ptr
#define MakeUnique std::make_unique
#define UniquePtr std::unique_ptr
#else
#define MakeShared dispatch::MakeSharedRefPtr
#define SharedPtr dispatch::SharedRefPtr
#define MakeBound dispatch::MakeBoundRefPtr
#define BoundPtr dispatch::BoundRefPtr
#define MakeUnique std::make_unique
#define UniquePtr std::unique_ptr
#endif

}