//===-- HTMLGenerator.cpp - HTML Generator ----------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Generators.h"
#include "Representation.h"
#include "support/File.h"
#include "clang/Basic/Version.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/StringSet.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include <algorithm>
#include <optional>
#include <string>

using namespace llvm;

namespace clang {
namespace doc {

namespace {

class HTMLTag {
public:
  // Any other tag can be added if required
  enum TagType {
    TAG_A,
    TAG_DIV,
    TAG_FOOTER,
    TAG_H1,
    TAG_H2,
    TAG_H3,
    TAG_HEADER,
    TAG_LI,
    TAG_LINK,
    TAG_MAIN,
    TAG_META,
    TAG_OL,
    TAG_P,
    TAG_SCRIPT,
    TAG_SPAN,
    TAG_TITLE,
    TAG_UL,
    TAG_TABLE,
    TAG_THEAD,
    TAG_TBODY,
    TAG_TR,
    TAG_TD,
    TAG_TH
  };

  HTMLTag() = default;
  constexpr HTMLTag(TagType Value) : Value(Value) {}

  operator TagType() const { return Value; }
  operator bool() = delete;

  bool isSelfClosing() const;
  StringRef toString() const;

private:
  TagType Value;
};

enum NodeType {
  NODE_TEXT,
  NODE_TAG,
};

struct HTMLNode {
  HTMLNode(NodeType Type) : Type(Type) {}
  virtual ~HTMLNode() = default;

  virtual void render(llvm::raw_ostream &OS, int IndentationLevel) = 0;
  NodeType Type; // Type of node
};

struct TextNode : public HTMLNode {
  TextNode(const Twine &Text)
      : HTMLNode(NodeType::NODE_TEXT), Text(Text.str()) {}

  std::string Text; // Content of node
  void render(llvm::raw_ostream &OS, int IndentationLevel) override;
};

struct TagNode : public HTMLNode {
  TagNode(HTMLTag Tag) : HTMLNode(NodeType::NODE_TAG), Tag(Tag) {}
  TagNode(HTMLTag Tag, const Twine &Text) : TagNode(Tag) {
    Children.emplace_back(std::make_unique<TextNode>(Text.str()));
  }

  HTMLTag Tag; // Name of HTML Tag (p, div, h1)
  std::vector<std::unique_ptr<HTMLNode>> Children; // List of child nodes
  std::vector<std::pair<std::string, std::string>>
      Attributes; // List of key-value attributes for tag

  void render(llvm::raw_ostream &OS, int IndentationLevel) override;
};

struct HTMLFile {
  std::vector<std::unique_ptr<HTMLNode>> Children; // List of child nodes
  void render(llvm::raw_ostream &OS) {
    OS << "<!DOCTYPE html>\n";
    for (const auto &C : Children) {
      C->render(OS, 0);
      OS << "\n";
    }
  }
};

} // namespace

bool HTMLTag::isSelfClosing() const {
  switch (Value) {
  case HTMLTag::TAG_META:
  case HTMLTag::TAG_LINK:
    return true;
  case HTMLTag::TAG_A:
  case HTMLTag::TAG_DIV:
  case HTMLTag::TAG_FOOTER:
  case HTMLTag::TAG_H1:
  case HTMLTag::TAG_H2:
  case HTMLTag::TAG_H3:
  case HTMLTag::TAG_HEADER:
  case HTMLTag::TAG_LI:
  case HTMLTag::TAG_MAIN:
  case HTMLTag::TAG_OL:
  case HTMLTag::TAG_P:
  case HTMLTag::TAG_SCRIPT:
  case HTMLTag::TAG_SPAN:
  case HTMLTag::TAG_TITLE:
  case HTMLTag::TAG_UL:
  case HTMLTag::TAG_TABLE:
  case HTMLTag::TAG_THEAD:
  case HTMLTag::TAG_TBODY:
  case HTMLTag::TAG_TR:
  case HTMLTag::TAG_TD:
  case HTMLTag::TAG_TH:
    return false;
  }
  llvm_unreachable("Unhandled HTMLTag::TagType");
}

StringRef HTMLTag::toString() const {
  switch (Value) {
  case HTMLTag::TAG_A:
    return "a";
  case HTMLTag::TAG_DIV:
    return "div";
  case HTMLTag::TAG_FOOTER:
    return "footer";
  case HTMLTag::TAG_H1:
    return "h1";
  case HTMLTag::TAG_H2:
    return "h2";
  case HTMLTag::TAG_H3:
    return "h3";
  case HTMLTag::TAG_HEADER:
    return "header";
  case HTMLTag::TAG_LI:
    return "li";
  case HTMLTag::TAG_LINK:
    return "link";
  case HTMLTag::TAG_MAIN:
    return "main";
  case HTMLTag::TAG_META:
    return "meta";
  case HTMLTag::TAG_OL:
    return "ol";
  case HTMLTag::TAG_P:
    return "p";
  case HTMLTag::TAG_SCRIPT:
    return "script";
  case HTMLTag::TAG_SPAN:
    return "span";
  case HTMLTag::TAG_TITLE:
    return "title";
  case HTMLTag::TAG_UL:
    return "ul";
  case HTMLTag::TAG_TABLE:
    return "table";
  case HTMLTag::TAG_THEAD:
    return "thead";
  case HTMLTag::TAG_TBODY:
    return "tbody";
  case HTMLTag::TAG_TR:
    return "tr";
  case HTMLTag::TAG_TD:
    return "td";
  case HTMLTag::TAG_TH:
    return "th";
  }
  llvm_unreachable("Unhandled HTMLTag::TagType");
}

void TextNode::render(llvm::raw_ostream &OS, int IndentationLevel) {
  OS.indent(IndentationLevel * 2);
  printHTMLEscaped(Text, OS);
}

void TagNode::render(llvm::raw_ostream &OS, int IndentationLevel) {
  // Children nodes are rendered in the same line if all of them are text nodes
  bool InlineChildren = true;
  for (const auto &C : Children)
    if (C->Type == NodeType::NODE_TAG) {
      InlineChildren = false;
      break;
    }
  OS.indent(IndentationLevel * 2);
  OS << "<" << Tag.toString();
  for (const auto &A : Attributes)
    OS << " " << A.first << "=\"" << A.second << "\"";
  if (Tag.isSelfClosing()) {
    OS << "/>";
    return;
  }
  OS << ">";
  if (!InlineChildren)
    OS << "\n";
  bool NewLineRendered = true;
  for (const auto &C : Children) {
    int ChildrenIndentation =
        InlineChildren || !NewLineRendered ? 0 : IndentationLevel + 1;
    C->render(OS, ChildrenIndentation);
    if (!InlineChildren && (C == Children.back() ||
                            (C->Type != NodeType::NODE_TEXT ||
                             (&C + 1)->get()->Type != NodeType::NODE_TEXT))) {
      OS << "\n";
      NewLineRendered = true;
    } else
      NewLineRendered = false;
  }
  if (!InlineChildren)
    OS.indent(IndentationLevel * 2);
  OS << "</" << Tag.toString() << ">";
}

template <typename Derived, typename Base,
          typename = std::enable_if<std::is_base_of<Derived, Base>::value>>
static void appendVector(std::vector<Derived> &&New,
                         std::vector<Base> &Original) {
  std::move(New.begin(), New.end(), std::back_inserter(Original));
}

// HTML generation

static std::vector<std::unique_ptr<TagNode>>
genStylesheetsHTML(StringRef InfoPath, const ClangDocContext &CDCtx) {
  std::vector<std::unique_ptr<TagNode>> Out;
  for (const auto &FilePath : CDCtx.UserStylesheets) {
    auto LinkNode = std::make_unique<TagNode>(HTMLTag::TAG_LINK);
    LinkNode->Attributes.emplace_back("rel", "stylesheet");
    SmallString<128> StylesheetPath = computeRelativePath("", InfoPath);
    llvm::sys::path::append(StylesheetPath,
                            llvm::sys::path::filename(FilePath));
    // Paths in HTML must be in posix-style
    llvm::sys::path::native(StylesheetPath, llvm::sys::path::Style::posix);
    LinkNode->Attributes.emplace_back("href", std::string(StylesheetPath));
    Out.emplace_back(std::move(LinkNode));
  }
  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genJsScriptsHTML(StringRef InfoPath, const ClangDocContext &CDCtx) {
  std::vector<std::unique_ptr<TagNode>> Out;

  // index_json.js is part of every generated HTML file
  SmallString<128> IndexJSONPath = computeRelativePath("", InfoPath);
  auto IndexJSONNode = std::make_unique<TagNode>(HTMLTag::TAG_SCRIPT);
  llvm::sys::path::append(IndexJSONPath, "index_json.js");
  llvm::sys::path::native(IndexJSONPath, llvm::sys::path::Style::posix);
  IndexJSONNode->Attributes.emplace_back("src", std::string(IndexJSONPath));
  Out.emplace_back(std::move(IndexJSONNode));

  for (const auto &FilePath : CDCtx.JsScripts) {
    SmallString<128> ScriptPath = computeRelativePath("", InfoPath);
    auto ScriptNode = std::make_unique<TagNode>(HTMLTag::TAG_SCRIPT);
    llvm::sys::path::append(ScriptPath, llvm::sys::path::filename(FilePath));
    // Paths in HTML must be in posix-style
    llvm::sys::path::native(ScriptPath, llvm::sys::path::Style::posix);
    ScriptNode->Attributes.emplace_back("src", std::string(ScriptPath));
    Out.emplace_back(std::move(ScriptNode));
  }
  return Out;
}

static std::unique_ptr<TagNode> genLink(const Twine &Text, const Twine &Link) {
  auto LinkNode = std::make_unique<TagNode>(HTMLTag::TAG_A, Text);
  LinkNode->Attributes.emplace_back("href", Link.str());
  return LinkNode;
}

static std::unique_ptr<HTMLNode>
genReference(const Reference &Type, StringRef CurrentDirectory,
             std::optional<StringRef> JumpToSection = std::nullopt) {
  if (Type.Path.empty()) {
    if (!JumpToSection)
      return std::make_unique<TextNode>(Type.Name);
    return genLink(Type.Name, "#" + *JumpToSection);
  }
  llvm::SmallString<64> Path = Type.getRelativeFilePath(CurrentDirectory);
  llvm::sys::path::append(Path, Type.getFileBaseName() + ".html");

  // Paths in HTML must be in posix-style
  llvm::sys::path::native(Path, llvm::sys::path::Style::posix);
  if (JumpToSection)
    Path += ("#" + *JumpToSection).str();
  return genLink(Type.Name, Path);
}

static std::vector<std::unique_ptr<HTMLNode>>
genReferenceList(const llvm::SmallVectorImpl<Reference> &Refs,
                 const StringRef &CurrentDirectory) {
  std::vector<std::unique_ptr<HTMLNode>> Out;
  for (const auto &R : Refs) {
    if (&R != Refs.begin())
      Out.emplace_back(std::make_unique<TextNode>(", "));
    Out.emplace_back(genReference(R, CurrentDirectory));
  }
  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const EnumInfo &I, const ClangDocContext &CDCtx);
static std::vector<std::unique_ptr<TagNode>>
genHTML(const FunctionInfo &I, const ClangDocContext &CDCtx,
        StringRef ParentInfoDir);
static std::unique_ptr<TagNode> genHTML(const std::vector<CommentInfo> &C);

static std::vector<std::unique_ptr<TagNode>>
genEnumsBlock(const std::vector<EnumInfo> &Enums,
              const ClangDocContext &CDCtx) {
  if (Enums.empty())
    return {};

  std::vector<std::unique_ptr<TagNode>> Out;
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_H2, "Enums"));
  Out.back()->Attributes.emplace_back("id", "Enums");
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_DIV));
  auto &DivBody = Out.back();
  for (const auto &E : Enums) {
    std::vector<std::unique_ptr<TagNode>> Nodes = genHTML(E, CDCtx);
    appendVector(std::move(Nodes), DivBody->Children);
  }
  return Out;
}

static std::unique_ptr<TagNode>
genEnumMembersBlock(const llvm::SmallVector<EnumValueInfo, 4> &Members) {
  if (Members.empty())
    return nullptr;

  auto List = std::make_unique<TagNode>(HTMLTag::TAG_TBODY);

  for (const auto &M : Members) {
    auto TRNode = std::make_unique<TagNode>(HTMLTag::TAG_TR);
    TRNode->Children.emplace_back(
        std::make_unique<TagNode>(HTMLTag::TAG_TD, M.Name));
    // Use user supplied value if it exists, otherwise use the value
    if (!M.ValueExpr.empty()) {
      TRNode->Children.emplace_back(
          std::make_unique<TagNode>(HTMLTag::TAG_TD, M.ValueExpr));
    } else {
      TRNode->Children.emplace_back(
          std::make_unique<TagNode>(HTMLTag::TAG_TD, M.Value));
    }
    if (!M.Description.empty()) {
      auto TD = std::make_unique<TagNode>(HTMLTag::TAG_TD);
      TD->Children.emplace_back(genHTML(M.Description));
      TRNode->Children.emplace_back(std::move(TD));
    }
    List->Children.emplace_back(std::move(TRNode));
  }
  return List;
}

static std::vector<std::unique_ptr<TagNode>>
genFunctionsBlock(const std::vector<FunctionInfo> &Functions,
                  const ClangDocContext &CDCtx, StringRef ParentInfoDir) {
  if (Functions.empty())
    return {};

  std::vector<std::unique_ptr<TagNode>> Out;
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_H2, "Functions"));
  Out.back()->Attributes.emplace_back("id", "Functions");
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_DIV));
  auto &DivBody = Out.back();
  for (const auto &F : Functions) {
    std::vector<std::unique_ptr<TagNode>> Nodes =
        genHTML(F, CDCtx, ParentInfoDir);
    appendVector(std::move(Nodes), DivBody->Children);
  }
  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genRecordMembersBlock(const llvm::SmallVector<MemberTypeInfo, 4> &Members,
                      StringRef ParentInfoDir) {
  if (Members.empty())
    return {};

  std::vector<std::unique_ptr<TagNode>> Out;
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_H2, "Members"));
  Out.back()->Attributes.emplace_back("id", "Members");
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_UL));
  auto &ULBody = Out.back();
  for (const auto &M : Members) {
    StringRef Access = getAccessSpelling(M.Access);
    auto LIBody = std::make_unique<TagNode>(HTMLTag::TAG_LI);
    auto MemberDecl = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
    if (!Access.empty())
      MemberDecl->Children.emplace_back(
          std::make_unique<TextNode>(Access + " "));
    if (M.IsStatic)
      MemberDecl->Children.emplace_back(std::make_unique<TextNode>("static "));
    MemberDecl->Children.emplace_back(genReference(M.Type, ParentInfoDir));
    MemberDecl->Children.emplace_back(std::make_unique<TextNode>(" " + M.Name));
    if (!M.Description.empty())
      LIBody->Children.emplace_back(genHTML(M.Description));
    LIBody->Children.emplace_back(std::move(MemberDecl));
    ULBody->Children.emplace_back(std::move(LIBody));
  }
  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genReferencesBlock(const std::vector<Reference> &References,
                   llvm::StringRef Title, StringRef ParentPath) {
  if (References.empty())
    return {};

  std::vector<std::unique_ptr<TagNode>> Out;
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_H2, Title));
  Out.back()->Attributes.emplace_back("id", std::string(Title));
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_UL));
  auto &ULBody = Out.back();
  for (const auto &R : References) {
    auto LiNode = std::make_unique<TagNode>(HTMLTag::TAG_LI);
    LiNode->Children.emplace_back(genReference(R, ParentPath));
    ULBody->Children.emplace_back(std::move(LiNode));
  }
  return Out;
}
static std::unique_ptr<TagNode> writeSourceFileRef(const ClangDocContext &CDCtx,
                                                   const Location &L) {

  if (!L.IsFileInRootDir && !CDCtx.RepositoryUrl)
    return std::make_unique<TagNode>(
        HTMLTag::TAG_P, "Defined at line " + std::to_string(L.StartLineNumber) +
                            " of file " + L.Filename);

  SmallString<128> FileURL(CDCtx.RepositoryUrl.value_or(""));
  llvm::sys::path::append(
      FileURL, llvm::sys::path::Style::posix,
      // If we're on Windows, the file name will be in the wrong format, and
      // append won't convert the full path being appended to the correct
      // format, so we need to do that here.
      llvm::sys::path::convert_to_slash(
          L.Filename,
          // The style here is the current style of the path, not the one we're
          // targeting. If the string is already in the posix style, it will do
          // nothing.
          llvm::sys::path::Style::windows));
  auto Node = std::make_unique<TagNode>(HTMLTag::TAG_P);
  Node->Children.emplace_back(std::make_unique<TextNode>("Defined at line "));
  auto LocNumberNode = std::make_unique<TagNode>(
      HTMLTag::TAG_A, std::to_string(L.StartLineNumber));
  // The links to a specific line in the source code use the github /
  // googlesource notation so it won't work for all hosting pages.
  LocNumberNode->Attributes.emplace_back(
      "href",
      formatv("{0}#{1}{2}", FileURL, CDCtx.RepositoryLinePrefix.value_or(""),
              L.StartLineNumber));
  Node->Children.emplace_back(std::move(LocNumberNode));
  Node->Children.emplace_back(std::make_unique<TextNode>(" of file "));
  auto LocFileNode = std::make_unique<TagNode>(
      HTMLTag::TAG_A, llvm::sys::path::filename(FileURL));
  LocFileNode->Attributes.emplace_back("href", std::string(FileURL));
  Node->Children.emplace_back(std::move(LocFileNode));
  return Node;
}

static void maybeWriteSourceFileRef(std::vector<std::unique_ptr<TagNode>> &Out,
                                    const ClangDocContext &CDCtx,
                                    const std::optional<Location> &DefLoc) {
  if (DefLoc)
    Out.emplace_back(writeSourceFileRef(CDCtx, *DefLoc));
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const Index &Index, StringRef InfoPath, bool IsOutermostList);

// Generates a list of child nodes for the HTML head tag
// It contains a meta node, link nodes to import CSS files, and script nodes to
// import JS files
static std::vector<std::unique_ptr<TagNode>>
genFileHeadNodes(StringRef Title, StringRef InfoPath,
                 const ClangDocContext &CDCtx) {
  std::vector<std::unique_ptr<TagNode>> Out;
  auto MetaNode = std::make_unique<TagNode>(HTMLTag::TAG_META);
  MetaNode->Attributes.emplace_back("charset", "utf-8");
  Out.emplace_back(std::move(MetaNode));
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_TITLE, Title));
  std::vector<std::unique_ptr<TagNode>> StylesheetsNodes =
      genStylesheetsHTML(InfoPath, CDCtx);
  appendVector(std::move(StylesheetsNodes), Out);
  std::vector<std::unique_ptr<TagNode>> JsNodes =
      genJsScriptsHTML(InfoPath, CDCtx);
  appendVector(std::move(JsNodes), Out);
  return Out;
}

// Generates a header HTML node that can be used for any file
// It contains the project name
static std::unique_ptr<TagNode> genFileHeaderNode(StringRef ProjectName) {
  auto HeaderNode = std::make_unique<TagNode>(HTMLTag::TAG_HEADER, ProjectName);
  HeaderNode->Attributes.emplace_back("id", "project-title");
  return HeaderNode;
}

// Generates a main HTML node that has all the main content of an info file
// It contains both indexes and the info's documented information
// This function should only be used for the info files (not for the file that
// only has the general index)
static std::unique_ptr<TagNode> genInfoFileMainNode(
    StringRef InfoPath,
    std::vector<std::unique_ptr<TagNode>> &MainContentInnerNodes,
    const Index &InfoIndex) {
  auto MainNode = std::make_unique<TagNode>(HTMLTag::TAG_MAIN);

  auto LeftSidebarNode = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
  LeftSidebarNode->Attributes.emplace_back("id", "sidebar-left");
  LeftSidebarNode->Attributes.emplace_back("path", std::string(InfoPath));
  LeftSidebarNode->Attributes.emplace_back(
      "class", "col-xs-6 col-sm-3 col-md-2 sidebar sidebar-offcanvas-left");

  auto MainContentNode = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
  MainContentNode->Attributes.emplace_back("id", "main-content");
  MainContentNode->Attributes.emplace_back(
      "class", "col-xs-12 col-sm-9 col-md-8 main-content");
  appendVector(std::move(MainContentInnerNodes), MainContentNode->Children);

  auto RightSidebarNode = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
  RightSidebarNode->Attributes.emplace_back("id", "sidebar-right");
  RightSidebarNode->Attributes.emplace_back(
      "class", "col-xs-6 col-sm-6 col-md-2 sidebar sidebar-offcanvas-right");
  std::vector<std::unique_ptr<TagNode>> InfoIndexHTML =
      genHTML(InfoIndex, InfoPath, true);
  appendVector(std::move(InfoIndexHTML), RightSidebarNode->Children);

  MainNode->Children.emplace_back(std::move(LeftSidebarNode));
  MainNode->Children.emplace_back(std::move(MainContentNode));
  MainNode->Children.emplace_back(std::move(RightSidebarNode));

  return MainNode;
}

// Generates a footer HTML node that can be used for any file
// It contains clang-doc's version
static std::unique_ptr<TagNode> genFileFooterNode() {
  auto FooterNode = std::make_unique<TagNode>(HTMLTag::TAG_FOOTER);
  auto SpanNode = std::make_unique<TagNode>(
      HTMLTag::TAG_SPAN, clang::getClangToolFullVersion("clang-doc"));
  SpanNode->Attributes.emplace_back("class", "no-break");
  FooterNode->Children.emplace_back(std::move(SpanNode));
  return FooterNode;
}

// Generates a complete HTMLFile for an Info
static HTMLFile
genInfoFile(StringRef Title, StringRef InfoPath,
            std::vector<std::unique_ptr<TagNode>> &MainContentNodes,
            const Index &InfoIndex, const ClangDocContext &CDCtx) {
  HTMLFile F;

  std::vector<std::unique_ptr<TagNode>> HeadNodes =
      genFileHeadNodes(Title, InfoPath, CDCtx);
  std::unique_ptr<TagNode> HeaderNode = genFileHeaderNode(CDCtx.ProjectName);
  std::unique_ptr<TagNode> MainNode =
      genInfoFileMainNode(InfoPath, MainContentNodes, InfoIndex);
  std::unique_ptr<TagNode> FooterNode = genFileFooterNode();

  appendVector(std::move(HeadNodes), F.Children);
  F.Children.emplace_back(std::move(HeaderNode));
  F.Children.emplace_back(std::move(MainNode));
  F.Children.emplace_back(std::move(FooterNode));

  return F;
}

template <typename T,
          typename = std::enable_if<std::is_base_of<T, Info>::value>>
static Index genInfoIndexItem(const std::vector<T> &Infos, StringRef Title) {
  Index Idx(Title, Title);
  for (const auto &C : Infos)
    Idx.Children.emplace_back(C.extractName(),
                              llvm::toHex(llvm::toStringRef(C.USR)));
  return Idx;
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const Index &Index, StringRef InfoPath, bool IsOutermostList) {
  std::vector<std::unique_ptr<TagNode>> Out;
  if (!Index.Name.empty()) {
    Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_SPAN));
    auto &SpanBody = Out.back();
    if (!Index.JumpToSection)
      SpanBody->Children.emplace_back(genReference(Index, InfoPath));
    else
      SpanBody->Children.emplace_back(
          genReference(Index, InfoPath, Index.JumpToSection->str()));
  }
  if (Index.Children.empty())
    return Out;
  // Only the outermost list should use ol, the others should use ul
  HTMLTag ListHTMLTag = IsOutermostList ? HTMLTag::TAG_OL : HTMLTag::TAG_UL;
  Out.emplace_back(std::make_unique<TagNode>(ListHTMLTag));
  const auto &UlBody = Out.back();
  for (const auto &C : Index.Children) {
    auto LiBody = std::make_unique<TagNode>(HTMLTag::TAG_LI);
    std::vector<std::unique_ptr<TagNode>> Nodes = genHTML(C, InfoPath, false);
    appendVector(std::move(Nodes), LiBody->Children);
    UlBody->Children.emplace_back(std::move(LiBody));
  }
  return Out;
}

static std::unique_ptr<HTMLNode> genHTML(const CommentInfo &I) {
  switch (I.Kind) {
  case CommentKind::CK_FullComment: {
    auto FullComment = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
    for (const auto &Child : I.Children) {
      std::unique_ptr<HTMLNode> Node = genHTML(*Child);
      if (Node)
        FullComment->Children.emplace_back(std::move(Node));
    }
    return std::move(FullComment);
  }

  case CommentKind::CK_ParagraphComment: {
    auto ParagraphComment = std::make_unique<TagNode>(HTMLTag::TAG_P);
    for (const auto &Child : I.Children) {
      std::unique_ptr<HTMLNode> Node = genHTML(*Child);
      if (Node)
        ParagraphComment->Children.emplace_back(std::move(Node));
    }
    if (ParagraphComment->Children.empty())
      return nullptr;
    return std::move(ParagraphComment);
  }

  case CommentKind::CK_BlockCommandComment: {
    auto BlockComment = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
    BlockComment->Children.emplace_back(
        std::make_unique<TagNode>(HTMLTag::TAG_DIV, I.Name));
    for (const auto &Child : I.Children) {
      std::unique_ptr<HTMLNode> Node = genHTML(*Child);
      if (Node)
        BlockComment->Children.emplace_back(std::move(Node));
    }
    if (BlockComment->Children.empty())
      return nullptr;
    return std::move(BlockComment);
  }

  case CommentKind::CK_TextComment: {
    if (I.Text.empty())
      return nullptr;
    return std::make_unique<TextNode>(I.Text);
  }

  // For now, return nullptr for unsupported comment kinds
  case CommentKind::CK_InlineCommandComment:
  case CommentKind::CK_HTMLStartTagComment:
  case CommentKind::CK_HTMLEndTagComment:
  case CommentKind::CK_ParamCommandComment:
  case CommentKind::CK_TParamCommandComment:
  case CommentKind::CK_VerbatimBlockComment:
  case CommentKind::CK_VerbatimBlockLineComment:
  case CommentKind::CK_VerbatimLineComment:
  case CommentKind::CK_Unknown:
    return nullptr;
  }
  llvm_unreachable("Unhandled CommentKind");
}

static std::unique_ptr<TagNode> genHTML(const std::vector<CommentInfo> &C) {
  auto CommentBlock = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
  for (const auto &Child : C) {
    if (std::unique_ptr<HTMLNode> Node = genHTML(Child))
      CommentBlock->Children.emplace_back(std::move(Node));
  }
  return CommentBlock;
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const EnumInfo &I, const ClangDocContext &CDCtx) {
  std::vector<std::unique_ptr<TagNode>> Out;
  std::string EnumType = I.Scoped ? "enum class " : "enum ";
  // Determine if enum members have comments attached
  bool HasComments = llvm::any_of(
      I.Members, [](const EnumValueInfo &M) { return !M.Description.empty(); });
  std::unique_ptr<TagNode> Table =
      std::make_unique<TagNode>(HTMLTag::TAG_TABLE);
  std::unique_ptr<TagNode> THead =
      std::make_unique<TagNode>(HTMLTag::TAG_THEAD);
  std::unique_ptr<TagNode> TRow = std::make_unique<TagNode>(HTMLTag::TAG_TR);
  std::unique_ptr<TagNode> TD =
      std::make_unique<TagNode>(HTMLTag::TAG_TH, EnumType + I.Name);
  // Span 3 columns if enum has comments
  TD->Attributes.emplace_back("colspan", HasComments ? "3" : "2");

  Table->Attributes.emplace_back("id", llvm::toHex(llvm::toStringRef(I.USR)));
  TRow->Children.emplace_back(std::move(TD));
  THead->Children.emplace_back(std::move(TRow));
  Table->Children.emplace_back(std::move(THead));

  if (std::unique_ptr<TagNode> Node = genEnumMembersBlock(I.Members))
    Table->Children.emplace_back(std::move(Node));

  Out.emplace_back(std::move(Table));

  maybeWriteSourceFileRef(Out, CDCtx, I.DefLoc);

  if (!I.Description.empty())
    Out.emplace_back(genHTML(I.Description));

  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const FunctionInfo &I, const ClangDocContext &CDCtx,
        StringRef ParentInfoDir) {
  std::vector<std::unique_ptr<TagNode>> Out;
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_H3, I.Name));
  // USR is used as id for functions instead of name to disambiguate function
  // overloads.
  Out.back()->Attributes.emplace_back("id",
                                      llvm::toHex(llvm::toStringRef(I.USR)));

  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_P));
  auto &FunctionHeader = Out.back();

  std::string Access = getAccessSpelling(I.Access).str();
  if (Access != "")
    FunctionHeader->Children.emplace_back(
        std::make_unique<TextNode>(Access + " "));
  if (I.IsStatic)
    FunctionHeader->Children.emplace_back(
        std::make_unique<TextNode>("static "));
  if (I.ReturnType.Type.Name != "") {
    FunctionHeader->Children.emplace_back(
        genReference(I.ReturnType.Type, ParentInfoDir));
    FunctionHeader->Children.emplace_back(std::make_unique<TextNode>(" "));
  }
  FunctionHeader->Children.emplace_back(
      std::make_unique<TextNode>(I.Name + "("));

  for (const auto &P : I.Params) {
    if (&P != I.Params.begin())
      FunctionHeader->Children.emplace_back(std::make_unique<TextNode>(", "));
    FunctionHeader->Children.emplace_back(genReference(P.Type, ParentInfoDir));
    FunctionHeader->Children.emplace_back(
        std::make_unique<TextNode>(" " + P.Name));
  }
  FunctionHeader->Children.emplace_back(std::make_unique<TextNode>(")"));

  maybeWriteSourceFileRef(Out, CDCtx, I.DefLoc);

  if (!I.Description.empty())
    Out.emplace_back(genHTML(I.Description));

  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const NamespaceInfo &I, Index &InfoIndex, const ClangDocContext &CDCtx,
        std::string &InfoTitle) {
  std::vector<std::unique_ptr<TagNode>> Out;
  if (I.Name.str() == "")
    InfoTitle = "Global Namespace";
  else
    InfoTitle = ("namespace " + I.Name).str();

  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_H1, InfoTitle));

  if (!I.Description.empty())
    Out.emplace_back(genHTML(I.Description));

  llvm::SmallString<64> BasePath = I.getRelativeFilePath("");

  std::vector<std::unique_ptr<TagNode>> ChildNamespaces =
      genReferencesBlock(I.Children.Namespaces, "Namespaces", BasePath);
  appendVector(std::move(ChildNamespaces), Out);
  std::vector<std::unique_ptr<TagNode>> ChildRecords =
      genReferencesBlock(I.Children.Records, "Records", BasePath);
  appendVector(std::move(ChildRecords), Out);

  std::vector<std::unique_ptr<TagNode>> ChildFunctions =
      genFunctionsBlock(I.Children.Functions, CDCtx, BasePath);
  appendVector(std::move(ChildFunctions), Out);
  std::vector<std::unique_ptr<TagNode>> ChildEnums =
      genEnumsBlock(I.Children.Enums, CDCtx);
  appendVector(std::move(ChildEnums), Out);

  if (!I.Children.Namespaces.empty())
    InfoIndex.Children.emplace_back("Namespaces", "Namespaces");
  if (!I.Children.Records.empty())
    InfoIndex.Children.emplace_back("Records", "Records");
  if (!I.Children.Functions.empty())
    InfoIndex.Children.emplace_back(
        genInfoIndexItem(I.Children.Functions, "Functions"));
  if (!I.Children.Enums.empty())
    InfoIndex.Children.emplace_back(
        genInfoIndexItem(I.Children.Enums, "Enums"));

  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const RecordInfo &I, Index &InfoIndex, const ClangDocContext &CDCtx,
        std::string &InfoTitle) {
  std::vector<std::unique_ptr<TagNode>> Out;
  InfoTitle = (getTagType(I.TagType) + " " + I.Name).str();
  Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_H1, InfoTitle));

  maybeWriteSourceFileRef(Out, CDCtx, I.DefLoc);

  if (!I.Description.empty())
    Out.emplace_back(genHTML(I.Description));

  std::vector<std::unique_ptr<HTMLNode>> Parents =
      genReferenceList(I.Parents, I.Path);
  std::vector<std::unique_ptr<HTMLNode>> VParents =
      genReferenceList(I.VirtualParents, I.Path);
  if (!Parents.empty() || !VParents.empty()) {
    Out.emplace_back(std::make_unique<TagNode>(HTMLTag::TAG_P));
    auto &PBody = Out.back();
    PBody->Children.emplace_back(std::make_unique<TextNode>("Inherits from "));
    if (Parents.empty())
      appendVector(std::move(VParents), PBody->Children);
    else if (VParents.empty())
      appendVector(std::move(Parents), PBody->Children);
    else {
      appendVector(std::move(Parents), PBody->Children);
      PBody->Children.emplace_back(std::make_unique<TextNode>(", "));
      appendVector(std::move(VParents), PBody->Children);
    }
  }

  std::vector<std::unique_ptr<TagNode>> Members =
      genRecordMembersBlock(I.Members, I.Path);
  appendVector(std::move(Members), Out);
  std::vector<std::unique_ptr<TagNode>> ChildRecords =
      genReferencesBlock(I.Children.Records, "Records", I.Path);
  appendVector(std::move(ChildRecords), Out);

  std::vector<std::unique_ptr<TagNode>> ChildFunctions =
      genFunctionsBlock(I.Children.Functions, CDCtx, I.Path);
  appendVector(std::move(ChildFunctions), Out);
  std::vector<std::unique_ptr<TagNode>> ChildEnums =
      genEnumsBlock(I.Children.Enums, CDCtx);
  appendVector(std::move(ChildEnums), Out);

  if (!I.Members.empty())
    InfoIndex.Children.emplace_back("Members", "Members");
  if (!I.Children.Records.empty())
    InfoIndex.Children.emplace_back("Records", "Records");
  if (!I.Children.Functions.empty())
    InfoIndex.Children.emplace_back(
        genInfoIndexItem(I.Children.Functions, "Functions"));
  if (!I.Children.Enums.empty())
    InfoIndex.Children.emplace_back(
        genInfoIndexItem(I.Children.Enums, "Enums"));

  return Out;
}

static std::vector<std::unique_ptr<TagNode>>
genHTML(const TypedefInfo &I, const ClangDocContext &CDCtx,
        std::string &InfoTitle) {
  // TODO support typedefs in HTML.
  return {};
}

/// Generator for HTML documentation.
class HTMLGenerator : public Generator {
public:
  static const char *Format;

  llvm::Error generateDocs(StringRef RootDir,
                           llvm::StringMap<std::unique_ptr<doc::Info>> Infos,
                           const ClangDocContext &CDCtx) override;
  llvm::Error createResources(ClangDocContext &CDCtx) override;
  llvm::Error generateDocForInfo(Info *I, llvm::raw_ostream &OS,
                                 const ClangDocContext &CDCtx) override;
};

const char *HTMLGenerator::Format = "html";

llvm::Error
HTMLGenerator::generateDocs(StringRef RootDir,
                            llvm::StringMap<std::unique_ptr<doc::Info>> Infos,
                            const ClangDocContext &CDCtx) {
  // Track which directories we already tried to create.
  llvm::StringSet<> CreatedDirs;

  // Collect all output by file name and create the nexessary directories.
  llvm::StringMap<std::vector<doc::Info *>> FileToInfos;
  for (const auto &Group : Infos) {
    doc::Info *Info = Group.getValue().get();

    llvm::SmallString<128> Path;
    llvm::sys::path::native(RootDir, Path);
    llvm::sys::path::append(Path, Info->getRelativeFilePath(""));
    if (!CreatedDirs.contains(Path)) {
      if (std::error_code Err = llvm::sys::fs::create_directories(Path);
          Err != std::error_code()) {
        return llvm::createStringError(Err, "Failed to create directory '%s'.",
                                       Path.c_str());
      }
      CreatedDirs.insert(Path);
    }

    llvm::sys::path::append(Path, Info->getFileBaseName() + ".html");
    FileToInfos[Path].push_back(Info);
  }

  for (const auto &Group : FileToInfos) {
    std::error_code FileErr;
    llvm::raw_fd_ostream InfoOS(Group.getKey(), FileErr,
                                llvm::sys::fs::OF_Text);
    if (FileErr) {
      return llvm::createStringError(FileErr, "Error opening file '%s'",
                                     Group.getKey().str().c_str());
    }

    // TODO: https://github.com/llvm/llvm-project/issues/59073
    // If there are multiple Infos for this file name (for example, template
    // specializations), this will generate multiple complete web pages (with
    // <DOCTYPE> and <title>, etc.) concatenated together. This generator needs
    // some refactoring to be able to output the headers separately from the
    // contents.
    for (const auto &Info : Group.getValue()) {
      if (llvm::Error Err = generateDocForInfo(Info, InfoOS, CDCtx)) {
        return Err;
      }
    }
  }

  return llvm::Error::success();
}

llvm::Error HTMLGenerator::generateDocForInfo(Info *I, llvm::raw_ostream &OS,
                                              const ClangDocContext &CDCtx) {
  std::string InfoTitle;
  std::vector<std::unique_ptr<TagNode>> MainContentNodes;
  Index InfoIndex;
  switch (I->IT) {
  case InfoType::IT_namespace:
    MainContentNodes = genHTML(*static_cast<clang::doc::NamespaceInfo *>(I),
                               InfoIndex, CDCtx, InfoTitle);
    break;
  case InfoType::IT_record:
    MainContentNodes = genHTML(*static_cast<clang::doc::RecordInfo *>(I),
                               InfoIndex, CDCtx, InfoTitle);
    break;
  case InfoType::IT_enum:
    MainContentNodes = genHTML(*static_cast<clang::doc::EnumInfo *>(I), CDCtx);
    break;
  case InfoType::IT_function:
    MainContentNodes =
        genHTML(*static_cast<clang::doc::FunctionInfo *>(I), CDCtx, "");
    break;
  case InfoType::IT_typedef:
    MainContentNodes =
        genHTML(*static_cast<clang::doc::TypedefInfo *>(I), CDCtx, InfoTitle);
    break;
  case InfoType::IT_concept:
  case InfoType::IT_variable:
  case InfoType::IT_friend:
    break;
  case InfoType::IT_default:
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "unexpected info type");
  }

  HTMLFile F = genInfoFile(InfoTitle, I->getRelativeFilePath(""),
                           MainContentNodes, InfoIndex, CDCtx);
  F.render(OS);

  return llvm::Error::success();
}

static std::string getRefType(InfoType IT) {
  switch (IT) {
  case InfoType::IT_default:
    return "default";
  case InfoType::IT_namespace:
    return "namespace";
  case InfoType::IT_record:
    return "record";
  case InfoType::IT_function:
    return "function";
  case InfoType::IT_enum:
    return "enum";
  case InfoType::IT_typedef:
    return "typedef";
  case InfoType::IT_concept:
    return "concept";
  case InfoType::IT_variable:
    return "variable";
  case InfoType::IT_friend:
    return "friend";
  }
  llvm_unreachable("Unknown InfoType");
}

static llvm::Error serializeIndex(ClangDocContext &CDCtx) {
  std::error_code OK;
  std::error_code FileErr;
  llvm::SmallString<128> FilePath;
  llvm::sys::path::native(CDCtx.OutDirectory, FilePath);
  llvm::sys::path::append(FilePath, "index_json.js");
  llvm::raw_fd_ostream OS(FilePath, FileErr, llvm::sys::fs::OF_Text);
  if (FileErr != OK) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "error creating index file: " +
                                       FileErr.message());
  }
  llvm::SmallString<128> RootPath(CDCtx.OutDirectory);
  if (llvm::sys::path::is_relative(RootPath)) {
    llvm::sys::fs::make_absolute(RootPath);
  }
  // Replace the escaped characters with a forward slash. It shouldn't matter
  // when rendering the webpage in a web browser. This helps to prevent the
  // JavaScript from escaping characters incorrectly, and introducing  bad paths
  // in the URLs.
  std::string RootPathEscaped = RootPath.str().str();
  llvm::replace(RootPathEscaped, '\\', '/');
  OS << "var RootPath = \"" << RootPathEscaped << "\";\n";

  llvm::SmallString<128> Base(CDCtx.Base);
  std::string BaseEscaped = Base.str().str();
  llvm::replace(BaseEscaped, '\\', '/');
  OS << "var Base = \"" << BaseEscaped << "\";\n";

  CDCtx.Idx.sort();
  llvm::json::OStream J(OS, 2);
  std::function<void(Index)> IndexToJSON = [&](const Index &I) {
    J.object([&] {
      J.attribute("USR", toHex(llvm::toStringRef(I.USR)));
      J.attribute("Name", I.Name);
      J.attribute("RefType", getRefType(I.RefType));
      J.attribute("Path", I.getRelativeFilePath(""));
      J.attributeArray("Children", [&] {
        for (const Index &C : I.Children)
          IndexToJSON(C);
      });
    });
  };
  OS << "async function LoadIndex() {\nreturn";
  IndexToJSON(CDCtx.Idx);
  OS << ";\n}";
  return llvm::Error::success();
}

// Generates a main HTML node that has the main content of the file that shows
// only the general index
// It contains the general index with links to all the generated files
static std::unique_ptr<TagNode> genIndexFileMainNode() {
  auto MainNode = std::make_unique<TagNode>(HTMLTag::TAG_MAIN);

  auto LeftSidebarNode = std::make_unique<TagNode>(HTMLTag::TAG_DIV);
  LeftSidebarNode->Attributes.emplace_back("id", "sidebar-left");
  LeftSidebarNode->Attributes.emplace_back("path", "");
  LeftSidebarNode->Attributes.emplace_back(
      "class", "col-xs-6 col-sm-3 col-md-2 sidebar sidebar-offcanvas-left");
  LeftSidebarNode->Attributes.emplace_back("style", "flex: 0 100%;");

  MainNode->Children.emplace_back(std::move(LeftSidebarNode));

  return MainNode;
}

static llvm::Error genIndex(const ClangDocContext &CDCtx) {
  std::error_code FileErr, OK;
  llvm::SmallString<128> IndexPath;
  llvm::sys::path::native(CDCtx.OutDirectory, IndexPath);
  llvm::sys::path::append(IndexPath, "index.html");
  llvm::raw_fd_ostream IndexOS(IndexPath, FileErr, llvm::sys::fs::OF_Text);
  if (FileErr != OK) {
    return llvm::createStringError(llvm::inconvertibleErrorCode(),
                                   "error creating main index: " +
                                       FileErr.message());
  }

  HTMLFile F;

  std::vector<std::unique_ptr<TagNode>> HeadNodes =
      genFileHeadNodes("Index", "", CDCtx);
  std::unique_ptr<TagNode> HeaderNode = genFileHeaderNode(CDCtx.ProjectName);
  std::unique_ptr<TagNode> MainNode = genIndexFileMainNode();
  std::unique_ptr<TagNode> FooterNode = genFileFooterNode();

  appendVector(std::move(HeadNodes), F.Children);
  F.Children.emplace_back(std::move(HeaderNode));
  F.Children.emplace_back(std::move(MainNode));
  F.Children.emplace_back(std::move(FooterNode));

  F.render(IndexOS);

  return llvm::Error::success();
}

llvm::Error HTMLGenerator::createResources(ClangDocContext &CDCtx) {
  auto Err = serializeIndex(CDCtx);
  if (Err)
    return Err;
  Err = genIndex(CDCtx);
  if (Err)
    return Err;

  for (const auto &FilePath : CDCtx.UserStylesheets) {
    Err = copyFile(FilePath, CDCtx.OutDirectory);
    if (Err)
      return Err;
  }
  for (const auto &FilePath : CDCtx.JsScripts) {
    Err = copyFile(FilePath, CDCtx.OutDirectory);
    if (Err)
      return Err;
  }
  return llvm::Error::success();
}

static GeneratorRegistry::Add<HTMLGenerator> HTML(HTMLGenerator::Format,
                                                  "Generator for HTML output.");

// This anchor is used to force the linker to link in the generated object
// file and thus register the generator.
volatile int HTMLGeneratorAnchorSource = 0;

} // namespace doc
} // namespace clang
