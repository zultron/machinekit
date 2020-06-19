#ifndef INTERP_TESTING_UTIL_HH
#define INTERP_TESTING_UTIL_HH

#include <stddef.h>
#include <rs274ngc_interp.hh>
#include <interp_return.hh>
#include <catch.hpp>
#include <mock_xyza_config.hh>
#include <cstdio>
#include <posemath_ext.hh>

using Catch::Matchers::WithinAbs;
#define INTERP_FUZZ 1e-10

#define REQUIRE_FUZZ(got, expected) REQUIRE_THAT(got, WithinAbs(expected, INTERP_FUZZ))
#define CHECK_FUZZ(got, expected) CHECK_THAT(got, WithinAbs(expected, INTERP_FUZZ))

#define REQUIRE_INTERP_OK(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) < INTERP_MIN_ERROR)
#define CHECK_INTERP_OK(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) < INTERP_MIN_ERROR)
#define REQUIRE_INTERP_ERROR(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) >= INTERP_MIN_ERROR)
#define CHECK_INTERP_ERROR(interp, interp_call) REQUIRE(print_interp_error((interp), (interp).interp_call, __FILE__, __LINE__) >= INTERP_MIN_ERROR)

#define REQUIRE_INTERP_EXEC_LINES_OK(interp, lines) REQUIRE(execute_lines_or_err(interp, lines, __FILE__, __LINE__) < INTERP_MIN_ERROR)

extern InterpBase *pinterp;
extern int emc_debug;

int print_interp_error(Interp &interp, int retval);
int print_interp_error(Interp & interp, int retval, const char* src_file, int line);

template <size_t N>
int execute_lines_or_err(Interp &interp, const char* (&lines)[N], const char *src_file, int line)
{
  for (auto l : lines) {
      auto res = (InterpReturn) print_interp_error(interp, interp.execute(l), src_file, line);
      if (isInterpError(res)) {
          return res;
      }
  }
  return INTERP_OK;
}

// Do a bunch of setup / assignments to force reinitialize SAI internals enough that sections won't step on each other
#define DECL_INIT_TEST_INTERP() \
    setup_xyza_config(); \
    SET_PARAMETER_FILE_NAME("unit_test.var"); \
    std::remove("unit_test.var"); \
    Interp test_interp = Interp(); \
    pinterp = &test_interp; \
    setup_struct * const settings = &test_interp._setup; \
    test_interp.init(); \
    settings->tool_table[0].toolno = 1; \
    settings->tool_table[1].toolno = 2; \
    settings->tool_table[2].toolno = 3 \

// TODO factor the setup into something more resuable for other configs
#define DECL_INIT_TEST_INTERP_9AXIS() \
    setup_9axis_config(); \
    SET_PARAMETER_FILE_NAME("unit_test.var"); \
    std::remove("unit_test.var"); \
    Interp test_interp = Interp(); \
    pinterp = &test_interp; \
    setup_struct * const settings = &test_interp._setup; \
    test_interp.init(); \
    settings->tool_table[0].toolno = 1; \
    settings->tool_table[1].toolno = 2; \
    settings->tool_table[2].toolno = 3 \


namespace Catch {
namespace Matchers {
namespace Array {

template<typename T, size_t N>
struct ApproxMatcher : Catch::MatcherBase<std::array<T, N>> {
    using ArrayType = std::array<T, N>;
    ApproxMatcher(ArrayType const& comparator) : m_comparator( comparator ) {}

    bool match(ArrayType const &v) const override {
        if (m_comparator.size() != v.size())
            return false;
        for (std::size_t i = 0; i < v.size(); ++i)
            if (m_comparator[i] != approx(v[i]))
                return false;
        return true;
    }
    std::string describe() const override {
        return "is approx: " + ::Catch::Detail::stringify( m_comparator );
    }
    template <typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    ApproxMatcher& epsilon( T const& newEpsilon ) {
        approx.epsilon(newEpsilon);
        return *this;
    }
    template <typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    ApproxMatcher& margin( T const& newMargin ) {
        approx.margin(newMargin);
        return *this;
    }
    template <typename = typename std::enable_if<std::is_constructible<double, T>::value>::type>
    ApproxMatcher& scale( T const& newScale ) {
        approx.scale(newScale);
        return *this;
    }

    ArrayType const& m_comparator;
    mutable Catch::Detail::Approx approx = Catch::Detail::Approx::custom();
};

// KLUDGE hard-coded size here
using PositionArrayApproxMatcher = ApproxMatcher<PositionArray, 9>;
}
    template<typename T, size_t N>
    Array::ApproxMatcher<T, N> Approx( std::array<T, N> const& comparator ) {
        return Array::ApproxMatcher<T, N>( comparator );
    }

    namespace PoseMath {

    struct CartApproxMatcher : Catch::MatcherBase<PM_CARTESIAN> {
        CartApproxMatcher(PM_CARTESIAN const& comparator) : m_comparator( comparator ) {}

        bool match(PM_CARTESIAN const &v) const override {
            if (m_comparator.x != approx(v.x))
                return false;
            if (m_comparator.y != approx(v.y))
                return false;
            if (m_comparator.x != approx(v.x))
                return false;
            return true;
        }
        std::string describe() const override {
            return "is approx: " + ::Catch::Detail::stringify( m_comparator );
        }
        CartApproxMatcher& epsilon(double const& newEpsilon ) {
            approx.epsilon(newEpsilon);
            return *this;
        }
        CartApproxMatcher& margin(double const& newMargin ) {
            approx.margin(newMargin);
            return *this;
        }
        CartApproxMatcher& scale(double const& newScale ) {
            approx.scale(newScale);
            return *this;
        }

        PM_CARTESIAN const& m_comparator;
        mutable Catch::Detail::Approx approx = Catch::Detail::Approx::custom();
    };
    }
}}

#endif // INTERP_TESTING_UTIL_HH
