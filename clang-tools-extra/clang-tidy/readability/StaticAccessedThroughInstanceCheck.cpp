//===--- StaticAccessedThroughInstanceCheck.cpp - clang-tidy---------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "StaticAccessedThroughInstanceCheck.h"
#include "clang/AST/ASTContext.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "llvm/ADT/StringRef.h"

using namespace clang::ast_matchers;

namespace clang::tidy::readability {

namespace {
AST_MATCHER(CXXMethodDecl, isStatic) { return Node.isStatic(); }
} // namespace

static unsigned getNameSpecifierNestingLevel(const QualType &QType) {
  if (const auto *ElType = QType->getAs<ElaboratedType>()) {
    if (const NestedNameSpecifier *NestedSpecifiers = ElType->getQualifier()) {
      unsigned NameSpecifierNestingLevel = 1;
      do {
        NameSpecifierNestingLevel++;
        NestedSpecifiers = NestedSpecifiers->getPrefix();
      } while (NestedSpecifiers);

      return NameSpecifierNestingLevel;
    }
  }
  return 0;
}

void StaticAccessedThroughInstanceCheck::storeOptions(
    ClangTidyOptions::OptionMap &Opts) {
  Options.store(Opts, "NameSpecifierNestingThreshold",
                NameSpecifierNestingThreshold);
}

void StaticAccessedThroughInstanceCheck::registerMatchers(MatchFinder *Finder) {
  Finder->addMatcher(
      memberExpr(hasDeclaration(anyOf(cxxMethodDecl(isStatic()),
                                      varDecl(hasStaticStorageDuration()),
                                      enumConstantDecl())))
          .bind("memberExpression"),
      this);
}

void StaticAccessedThroughInstanceCheck::check(
    const MatchFinder::MatchResult &Result) {
  const auto *MemberExpression =
      Result.Nodes.getNodeAs<MemberExpr>("memberExpression");

  if (MemberExpression->getBeginLoc().isMacroID())
    return;

  const Expr *BaseExpr = MemberExpression->getBase();

  const QualType BaseType =
      BaseExpr->getType()->isPointerType()
          ? BaseExpr->getType()->getPointeeType().getUnqualifiedType()
          : BaseExpr->getType().getUnqualifiedType();

  const ASTContext *AstContext = Result.Context;
  PrintingPolicy PrintingPolicyWithSuppressedTag(AstContext->getLangOpts());
  PrintingPolicyWithSuppressedTag.SuppressTagKeyword = true;
  PrintingPolicyWithSuppressedTag.SuppressUnwrittenScope = true;

  PrintingPolicyWithSuppressedTag.PrintAsCanonical =
      !BaseExpr->getType()->isTypedefNameType();

  std::string BaseTypeName =
      BaseType.getAsString(PrintingPolicyWithSuppressedTag);

  // Ignore anonymous structs/classes which will not have an identifier
  const RecordDecl *RecDecl = BaseType->getAsCXXRecordDecl();
  if (!RecDecl || RecDecl->getIdentifier() == nullptr)
    return;

  // Do not warn for CUDA built-in variables.
  if (StringRef(BaseTypeName).starts_with("__cuda_builtin_"))
    return;

  SourceLocation MemberExprStartLoc = MemberExpression->getBeginLoc();
  auto CreateFix = [&] {
    return FixItHint::CreateReplacement(
        CharSourceRange::getCharRange(MemberExprStartLoc,
                                      MemberExpression->getMemberLoc()),
        BaseTypeName + "::");
  };

  {
    auto Diag =
        diag(MemberExprStartLoc, "static member accessed through instance");

    if (getNameSpecifierNestingLevel(BaseType) > NameSpecifierNestingThreshold)
      return;

    if (!BaseExpr->HasSideEffects(*AstContext,
                                  /* IncludePossibleEffects =*/true)) {
      Diag << CreateFix();
      return;
    }
  }

  diag(MemberExprStartLoc, "member base expression may carry some side effects",
       DiagnosticIDs::Level::Note)
      << BaseExpr->getSourceRange() << CreateFix();
}

} // namespace clang::tidy::readability
