#pragma once

#include <coroutine>
#include <memory>

namespace Coro {
	// TODO C++23 replace with std::generator
	template<typename T>
	struct [[nodiscard]] generator {
		struct promise_type {
			const T* value;
			std::exception_ptr exception;

			generator get_return_object() noexcept { return generator{ *this }; }
			std::suspend_always initial_suspend() const noexcept { return {}; }
			std::suspend_always final_suspend() const noexcept { return {}; }
			void unhandled_exception() noexcept { exception = std::current_exception(); }
			void return_void() const noexcept { }
			std::suspend_always yield_value(const T& valRef) noexcept
			{
				value = std::addressof(valRef);
				return {};
			}
		};

		using Handle = std::coroutine_handle<promise_type>;

		struct iterator {
			using iterator_category = std::input_iterator_tag;
			using difference_type = ptrdiff_t;
			using value_type = T;
			using reference = const T&;
			using pointer = const T*;

			Handle handle = nullptr;

			iterator& operator++()
			{
				handle.resume();
				if (handle.done()) {
					auto exception = std::exchange(handle.promise().exception, nullptr);
					handle = nullptr;
					if (exception)
						std::rethrow_exception(exception);
				}
				return *this;
			}

			[[nodiscard]] bool operator==(const iterator&) const = default;
			[[nodiscard]] reference operator*() const noexcept { return *handle.promise().value; }
			[[nodiscard]] pointer operator->() const noexcept { return handle.promise().value; }
		};

		[[nodiscard]] iterator begin()
		{
			if (!handle || handle.done()) // already iterated, quietly make empty sequence
				return iterator{};
			iterator it{ handle };
			++it;
			return it;
		}

		[[nodiscard]] iterator end() noexcept { return iterator{}; }

		explicit generator(promise_type& promise) noexcept
			: handle(Handle::from_promise(promise))
		{
		}

		generator() = default;

		generator(generator&& that) noexcept
			: handle(std::exchange(that.handle, nullptr))
		{
		}

		generator& operator=(generator&& that) noexcept
		{
			handle = std::exchange(that.handle, nullptr);
			return *this;
		}

		~generator()
		{
			if (handle)
				handle.destroy();
		}

	private:
		Handle handle = nullptr;
	};
}
