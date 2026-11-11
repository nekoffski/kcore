#pragma once

#include <concepts>
#include <functional>
#include <kcore/Error.hh>
#include <type_traits>
#include <utility>

namespace kc {
namespace details {

template <typename T>
inline constexpr bool isPipelineResult = false;

template <typename T>
inline constexpr bool isPipelineResult<Result<T>> = true;

template <typename R>
concept PipelineResult = isPipelineResult<std::remove_cvref_t<R>>;

template <typename R, typename F>
concept PipelineCallable =
    (std::is_void_v<typename std::remove_cvref_t<R>::value_type> &&
     std::invocable<F>) ||
    (not std::is_void_v<typename std::remove_cvref_t<R>::value_type> &&
     std::invocable<F, decltype(*std::declval<R>())>);

template <typename R, typename F>
using PipelineStepResult = typename std::conditional_t<
    std::is_void_v<typename std::remove_cvref_t<R>::value_type>,
    std::invoke_result<F>,
    std::invoke_result<F, decltype(*std::declval<R>())>>::type;

template <typename R, typename F>
concept PipelineThen =
    PipelineCallable<R, F> && PipelineResult<PipelineStepResult<R, F>>;

template <typename R, typename F>
concept PipelineTransform =
    PipelineCallable<R, F> && not PipelineResult<PipelineStepResult<R, F>> &&
    not std::is_reference_v<PipelineStepResult<R, F>>;

template <typename R, typename F>
concept PipelineMapError =
    std::invocable<F, decltype(std::declval<R>().error())> &&
    std::same_as<
        std::invoke_result_t<F, decltype(std::declval<R>().error())>, Error>;

template <typename R>
concept PipelinePreservable =
    std::is_void_v<typename std::remove_cvref_t<R>::value_type> ||
    std::is_constructible_v<
        typename std::remove_cvref_t<R>::value_type,
        decltype(*std::declval<R>())>;

template <typename R, typename F>
concept PipelineRecover =
    std::invocable<F, decltype(std::declval<R>().error())> &&
    PipelineResult<
        std::invoke_result_t<F, decltype(std::declval<R>().error())>> &&
    std::same_as<
        typename std::remove_cvref_t<std::invoke_result_t<
            F, decltype(std::declval<R>().error())>>::value_type,
        typename std::remove_cvref_t<R>::value_type> &&
    PipelinePreservable<R>;

template <typename F>
concept PipelineStart =
    std::invocable<F> && PipelineResult<std::invoke_result_t<F>>;

struct PipelineFromInvoke {};

}  // namespace details

template <typename T>
class [[nodiscard]] Pipeline {
   public:
    using value_type = T;

    template <typename F>
        requires details::PipelineThen<Result<T>&, F>
    [[nodiscard]] constexpr auto then(F&& f) & {
        return next([&] { return m_result.and_then(std::forward<F>(f)); });
    }

    template <typename F>
        requires details::PipelineThen<const Result<T>&, F>
    [[nodiscard]] constexpr auto then(F&& f) const& {
        return next([&] { return m_result.and_then(std::forward<F>(f)); });
    }

    template <typename F>
        requires details::PipelineThen<Result<T>&&, F>
    [[nodiscard]] constexpr auto then(F&& f) && {
        return next([&] {
            return std::move(m_result).and_then(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineThen<const Result<T>&&, F>
    [[nodiscard]] constexpr auto then(F&& f) const&& {
        return next([&] {
            return std::move(m_result).and_then(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineTransform<Result<T>&, F>
    [[nodiscard]] constexpr auto transform(F&& f) & {
        return next([&] { return m_result.transform(std::forward<F>(f)); });
    }

    template <typename F>
        requires details::PipelineTransform<const Result<T>&, F>
    [[nodiscard]] constexpr auto transform(F&& f) const& {
        return next([&] { return m_result.transform(std::forward<F>(f)); });
    }

    template <typename F>
        requires details::PipelineTransform<Result<T>&&, F>
    [[nodiscard]] constexpr auto transform(F&& f) && {
        return next([&] {
            return std::move(m_result).transform(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineTransform<const Result<T>&&, F>
    [[nodiscard]] constexpr auto transform(F&& f) const&& {
        return next([&] {
            return std::move(m_result).transform(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineCallable<const Result<T>&, F> &&
                 std::same_as<
                     details::PipelineStepResult<const Result<T>&, F>,
                     Result<void>> &&
                 std::is_copy_constructible_v<Result<T>>
    [[nodiscard]] constexpr auto check(F&& f) const& {
        return preserve(m_result, std::forward<F>(f));
    }

    template <typename F>
        requires details::PipelineCallable<const Result<T>&, F> &&
                 std::same_as<
                     details::PipelineStepResult<const Result<T>&, F>,
                     Result<void>> &&
                 std::is_move_constructible_v<Result<T>>
    [[nodiscard]] constexpr auto check(F&& f) && {
        return preserve(std::move(m_result), std::forward<F>(f));
    }

    template <typename F>
        requires details::PipelineCallable<const Result<T>&, F> &&
                 std::same_as<
                     details::PipelineStepResult<const Result<T>&, F>, void> &&
                 std::is_copy_constructible_v<Result<T>>
    [[nodiscard]] constexpr auto inspect(F&& f) const& {
        return preserve(m_result, std::forward<F>(f));
    }

    template <typename F>
        requires details::PipelineCallable<const Result<T>&, F> &&
                 std::same_as<
                     details::PipelineStepResult<const Result<T>&, F>, void> &&
                 std::is_move_constructible_v<Result<T>>
    [[nodiscard]] constexpr auto inspect(F&& f) && {
        return preserve(std::move(m_result), std::forward<F>(f));
    }

    template <typename F>
        requires details::PipelineMapError<Result<T>&, F> &&
                 details::PipelinePreservable<Result<T>&>
    [[nodiscard]] constexpr auto mapError(F&& f) & {
        return next([&] {
            return m_result.transform_error(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineMapError<const Result<T>&, F> &&
                 details::PipelinePreservable<const Result<T>&>
    [[nodiscard]] constexpr auto mapError(F&& f) const& {
        return next([&] {
            return m_result.transform_error(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineMapError<Result<T>&&, F> &&
                 details::PipelinePreservable<Result<T>&&>
    [[nodiscard]] constexpr auto mapError(F&& f) && {
        return next([&] {
            return std::move(m_result).transform_error(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineMapError<const Result<T>&&, F> &&
                 details::PipelinePreservable<const Result<T>&&>
    [[nodiscard]] constexpr auto mapError(F&& f) const&& {
        return next([&] {
            return std::move(m_result).transform_error(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineRecover<Result<T>&, F>
    [[nodiscard]] constexpr auto recover(F&& f) & {
        return next([&] { return m_result.or_else(std::forward<F>(f)); });
    }

    template <typename F>
        requires details::PipelineRecover<const Result<T>&, F>
    [[nodiscard]] constexpr auto recover(F&& f) const& {
        return next([&] { return m_result.or_else(std::forward<F>(f)); });
    }

    template <typename F>
        requires details::PipelineRecover<Result<T>&&, F>
    [[nodiscard]] constexpr auto recover(F&& f) && {
        return next([&] {
            return std::move(m_result).or_else(std::forward<F>(f));
        });
    }

    template <typename F>
        requires details::PipelineRecover<const Result<T>&&, F>
    [[nodiscard]] constexpr auto recover(F&& f) const&& {
        return next([&] {
            return std::move(m_result).or_else(std::forward<F>(f));
        });
    }

    [[nodiscard]] constexpr Result<T> result() const&
        requires std::is_copy_constructible_v<Result<T>>
    {
        return m_result;
    }

    [[nodiscard]] constexpr Result<T> result() &&
        requires std::is_move_constructible_v<Result<T>>
    {
        return std::move(m_result);
    }

   private:
    template <typename>
    friend class Pipeline;

    template <typename F>
        requires details::PipelineStart<F>
    friend constexpr auto startPipeline(F&& first);

    template <typename F>
    constexpr Pipeline(details::PipelineFromInvoke, F&& f)
        : m_result(std::invoke(std::forward<F>(f))) {}

    template <typename F>
    static constexpr auto next(F&& f) {
        using R = std::invoke_result_t<F>;
        return Pipeline<typename R::value_type>{
            details::PipelineFromInvoke{}, std::forward<F>(f)
        };
    }

    template <typename R, typename F>
    static constexpr auto preserve(R&& result, F&& f) {
        return Pipeline{
            details::PipelineFromInvoke{}, [&]() -> Result<T> {
                if (not result) {
                    return std::unexpected{std::forward<R>(result).error()};
                }
                const auto invoke = [&]() -> decltype(auto) {
                    if constexpr (std::is_void_v<T>) {
                        return std::invoke(std::forward<F>(f));
                    } else {
                        return std::invoke(
                            std::forward<F>(f), std::as_const(*result)
                        );
                    }
                };
                if constexpr (
                    std::is_void_v<std::invoke_result_t<decltype(invoke)>>
                ) {
                    invoke();
                } else {
                    auto r = invoke();
                    if (not r) {
                        return std::unexpected{std::move(r).error()};
                    }
                }
                return std::forward<R>(result);
            }
        };
    }

    Result<T> m_result;
};

template <typename F>
    requires details::PipelineStart<F>
[[nodiscard]] constexpr auto startPipeline(F&& first) {
    using R = std::remove_cvref_t<std::invoke_result_t<F>>;
    return Pipeline<typename R::value_type>{
        details::PipelineFromInvoke{}, std::forward<F>(first)
    };
}

}  // namespace kc
