#include <gtest/gtest.h>

#include <numbers>
#include <vector>
#include <list>
#include <thread>
#include <chrono>
#include <typeinfo>
#include <map>
#include <unordered_map>
#include <utils/proxy.hpp>
#include <utils/format.hpp>

using namespace gal::utils;

using scalar_type = double;
#define SCALAR_FORMAT "{:<16}"

struct drawable : as_interface<std::string()>
{
	template<typename T>
	std::string operator()(const T& self) { return self.draw(); }
};

struct calculable : as_interface<scalar_type()>
{
	template<typename T>
	scalar_type operator()(const T& self) { return self.calculate(); }
};

struct drawable_and_calculable : as_facade<drawable, calculable> {};

struct rectangle
{
	scalar_type width;
	scalar_type height;

	[[nodiscard]] std::string draw() const noexcept { return std_format::format("rectangle: [" SCALAR_FORMAT ", " SCALAR_FORMAT "]", width, height); }

	[[nodiscard]] scalar_type calculate() const noexcept { return width * height; }
};

struct circle
{
	scalar_type radius;

	[[nodiscard]] std::string draw() const noexcept { return std_format::format("circle: (" SCALAR_FORMAT ")", radius); }

	[[nodiscard]] scalar_type calculate() const noexcept { return std::numbers::pi * radius * radius; }
};

void draw_and_calculate(proxy<drawable_and_calculable> p) { std::cout << std_format::format("{} -> area: " SCALAR_FORMAT "\n", p.invoke<drawable>(), p.invoke<calculable>()); }

TEST(TestProxy, TestBasicShapeProxy)
{
	rectangle r{std::numbers::ln2, std::numbers::log10e};
	circle c{std::numbers::e};

	draw_and_calculate(make_proxy<drawable_and_calculable>(r));
	draw_and_calculate(make_proxy<drawable_and_calculable>(c));
}

template<typename Signature>
struct callable;

template<typename Return, typename... Args>
struct callable<Return(Args ...)> : as_interface<Return(Args&&...)>
{
	template<typename Function>
	decltype(auto) operator()(Function& function, Args&&... args) { return std::invoke(function, std::forward<Args>(args)...); }
};

template<typename Signature>
struct callable_facade : as_facade<callable<Signature>> {};

template<typename ValueType>
struct iterable : as_interface<void(proxy<callable_facade<void(ValueType&)>>)>
{
	template<typename Container>
	void operator()(Container& container, proxy<callable_facade<void(ValueType&)>>&& function)
	{
		std::ranges::for_each(
				container,
				[&function](ValueType& value) { function.invoke(value); });
	}
};

template<typename ValueType>
struct iterable_facade : as_facade<iterable<ValueType>> {};

template<typename ValueType>
void print(proxy<iterable_facade<ValueType>> p)
{
	auto function = [](const ValueType& v) { std::cout << v << '\t'; };

	p.invoke(+function);
	std::cout << '\n';
}

template<typename ValueType>
void make_double(proxy<iterable_facade<ValueType>> p)
{
	auto function = [](ValueType& v) { v *= 2; };

	p.invoke(+function);
}

TEST(TestProxy, TestContainerProxy)
{
	std::vector vec{1, 2, 3, 4, 5};
	std::list list{9, 8, 7, 6, 5};

	print<int>(&vec);
	print<int>(&list);
	make_double<int>(&vec);
	make_double<int>(&list);
	print<int>(&vec);
	print<int>(&list);
}

struct initializer : as_interface<void(std::size_t)>
{
	template<typename T>
	void operator()(T& self, const std::size_t capacity) { self.reserve(capacity); }
};

struct cleaner : as_interface<void()>
{
	template<typename T>
	void operator()(T& self) { self.clear(); }
};

struct checker : as_interface<bool()>
{
	template<typename T>
	bool operator()(const T& self) { return self.canceled(); }
};

struct ticker : as_interface<void(std::size_t)>
{
	template<typename T>
	void operator()(T& self, const std::size_t tick) { self.emplace_back(tick); }
};

struct exception_handler : as_interface<void(std::exception_ptr)>
{
	template<typename T>
	void operator()(T& self, std::exception_ptr&& e) { self.handle_exception(std::move(e)); }
};

struct clock_facade
		: as_facade<
			initializer,
			cleaner,
			checker,
			ticker,
			exception_handler> {};

void use_clock(proxy<clock_facade> p)
{
	try
	{
		constexpr std::size_t total_seconds = 600;
		using namespace std::chrono_literals;

		std::this_thread::sleep_for(1s);
		p.invoke<initializer>(total_seconds);

		for (std::size_t i = 0; i < total_seconds; ++i)
		{
			std::this_thread::sleep_for(20ms);
			if (p.invoke<checker>()) { throw std::runtime_error{"the clock is canceled"}; }

			p.invoke<ticker>((((i % 42) + 4) * 2) - 42);
		}
	}
	catch (...) { p.invoke<exception_handler>(std::current_exception()); }
}

struct my_clock
{
private:
	mutable std::mutex mutex_;
	std::vector<std::size_t> ticks_;
	std::size_t capacity_ = 0;
	bool canceled_ = false;
	std::exception_ptr current_exception_;

public:
	void reserve(const std::size_t capacity)
	{
		std::scoped_lock lock{mutex_};
		capacity_ = capacity;
		ticks_.reserve(capacity);
	}

	void clear() noexcept
	{
		std::scoped_lock lock{mutex_};
		ticks_.clear();
	}

	[[nodiscard]] bool canceled() const noexcept
	{
		std::scoped_lock lock{mutex_};
		return canceled_;
	}

	void cancel() noexcept
	{
		std::scoped_lock lock{mutex_};
		canceled_ = true;
	}

	void emplace_back(const std::size_t tick)
	{
		std::scoped_lock lock{mutex_};
		ticks_.emplace_back(tick);
	}

	void handle_exception(const std::exception_ptr& e) noexcept
	{
		std::scoped_lock lock{mutex_};
		current_exception_ = e;
	}

private:
	[[nodiscard]] bool is_initialized() const
	{
		std::scoped_lock lock{mutex_};
		if (current_exception_) { std::rethrow_exception(current_exception_); }
		return capacity_ != 0;
	}

	[[nodiscard]] std::pair<std::size_t, std::size_t> last_tick() const noexcept
	{
		std::scoped_lock lock{mutex_};
		if (current_exception_) { std::rethrow_exception(current_exception_); }
		return {ticks_.size(), ticks_.back()};
	}

public:
	void run() const
	{
		try
		{
			using namespace std::chrono_literals;

			while (not is_initialized())
			{
				std::cout << "Initializing...\n";
				std::this_thread::sleep_for(100ms);
			}
			std::cout << "Initialization successful!\n";

			while (true)
			{
				std::this_thread::sleep_for(100ms);

				const auto [size, back] = last_tick();
				if (size == capacity_) { break; }

				std::cout << std_format::format("{} tick(s) is currently stored, and the last tick value is {}...\n", size, back);
			}
		}
		catch (const std::exception& e) { std::cout << std_format::format("Something went wrong! => {}\n", e.what()); }
	}
};

TEST(TestProxy, TestThreadProxy)
{
	// auto c = std::make_shared<my_clock>();
	//
	// std::jthread t1{use_clock, c};
	// std::jthread t2{[c]
	// {
	// 	std::this_thread::sleep_for(std::chrono::seconds(3));
	// 	std::cout << "Try turning off the clock...\n";
	// 	c->cancel();
	// }};
	//
	// c->run();
}

struct reflection_type_info
{
	std::reference_wrapper<const std::type_info> type;

	template<typename T>
	constexpr explicit reflection_type_info(std::in_place_type_t<T>)
		: type{typeid(T)} { }

	[[nodiscard]] decltype(auto) name() const noexcept { return type.get().name(); }
};

struct positionable : as_interface<std::string_view(std::size_t)>
{
	template<typename T>
	std::string_view operator()(const T& self, const std::size_t key) { return self.at(key); }
};

struct positionable_facade : as_facade<positionable>
{
	using reflection_type = reflection_type_info;
	constexpr static std::size_t maximum_size = sizeof(std::optional<std::vector<std::string>>);
	constexpr static proxy_detail::constraint_level minimum_copyable_level = proxy_detail::constraint_level::nontrivial;
};

void location_target(proxy<positionable_facade> p)
{
	try
	{
		std::cout << std_format::format(
				"Trying to locate the target element '{}'(type: {}) with key '{}'\n",
				p.invoke(42),
				p.reflect().name(),
				42);
	}
	catch (const std::out_of_range& e) { std::cout << std_format::format("The target element with key {} does not exist. => {}\n", 42, e.what()); }
}

TEST(TestProxy, TestCustomFacadeProxy)
{
	std::map<std::size_t, std::string> map1{
			{1, "hello"},
			{2, "world"},
			{3, "triple"},
			{4, "nothing"},
			{42, "answer"}};

	std::unordered_map<std::size_t, std::string> map2{
			{42, "ouch"},
			{42, ","},
			{42, "there"},
			{42, "are"},
			{42, "so"},
			{42, "many"},
			{42, "answers"},
			{0, "!"}
	};

	std::vector<std::string> vec{"no", "answer", "here", "!"};
	static_assert(sizeof(decltype(map1)));
	static_assert(sizeof(decltype(map2)));
	static_assert(sizeof(decltype(vec)));
	location_target(&map1);
	location_target(&map2);
	location_target(std::make_optional(vec));
	location_target(std::make_shared<std::map<std::size_t, std::string>>());
}
