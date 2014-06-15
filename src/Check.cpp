#include "rapidcheck/Check.h"

#include "rapidcheck/detail/ImplicitParam.h"
#include "rapidcheck/detail/GenerationParams.h"
#include "rapidcheck/detail/Rose.h"

namespace rc {
namespace detail {

template<typename Callable>
auto withTestCase(const TestCase &testCase, Callable callable)
    -> decltype(callable())
{
    ImplicitParam<param::RandomEngine> randomEngine;
    randomEngine.let(RandomEngine());
    randomEngine->seed(testCase.seed);
    ImplicitParam<param::Size> size;
    size.let(testCase.size);
    ImplicitParam<param::NoShrink> noShrink;
    noShrink.let(false);

    return callable();
}


TestResult shrinkFailingCase(const gen::GeneratorUP<CaseResult> &property,
                             const TestCase &testCase)
{
    return withTestCase(testCase, [&]{
        RoseNode rootNode;
        FailureResult result;
        result.failingCase = testCase;
        auto shrinkResult = rootNode.shrink(*property);
        result.description = std::get<0>(shrinkResult).description();
        result.numShrinks = std::get<1>(shrinkResult);
        result.counterExample = rootNode.example();
        return result;
    });
}

//! Describes the parameters for a test.
struct TestParams
{
    //! The maximum number of successes before deciding a property passes.
    int maxSuccess = 100;
    //! The maximum size to generate.
    size_t maxSize = 100;
};

TestResult checkProperty(const gen::GeneratorUP<CaseResult> &property)
{
    using namespace detail;
    TestParams params;
    TestCase currentCase;
    RandomEngine seedEngine;

    currentCase.size = 0;
    for (currentCase.index = 1;
         currentCase.index <= params.maxSuccess;
         currentCase.index++)
    {
        currentCase.seed = seedEngine.nextAtom();

        CaseResult result = withTestCase(
            currentCase,
            [&]{ return (*property)(); });

        if (result.type() == CaseResult::Type::Failure)
            return shrinkFailingCase(property, currentCase);

        currentCase.size = std::min(params.maxSize, currentCase.size + 1);
        //TODO better size calculation
    }

    return SuccessResult{ .numTests = currentCase.index };
}

void assertTrue(bool condition,
                std::string description,
                std::string file,
                int line)
{
    if (!condition) {
        throw CaseResult(
            CaseResult::Type::Failure,
            file + ":" + std::to_string(line) + ": " + description);
    }
}

} // namespace detail
} // namespace rc