#include "pch.h"
#ifdef USE_ONNX
#include "TextEmbedder.h"
#include "Prompts/PromptScheduler.h"

using namespace Axodox::Infrastructure;
using namespace Axodox::MachineLearning::Prompts;
using namespace std;

namespace Axodox::MachineLearning
{
  TextEmbedder::TextEmbedder(OnnxEnvironment& environment, const std::filesystem::path& sourcePath) :
    _textTokenizer(environment, sourcePath),
    _textEncoder(environment)
  { }

  std::vector<std::shared_ptr<Tensor>> TextEmbedder::ScheduleText(std::string_view text, uint32_t stepCount)
  {
    auto prompts = SchedulePrompt(text, stepCount);
    
    unordered_map<string, shared_ptr<Tensor>> embeddingsByPrompt;
    vector<shared_ptr<Tensor>> embeddings;
    embeddings.reserve(stepCount);
    for (auto& prompt : prompts)
    {
      auto& embedding = embeddingsByPrompt[prompt];
      if (!embedding)
      {
        embedding = make_shared<Tensor>(ProcessText(prompt));
      }

      embeddings.push_back(embedding);
    }

    return embeddings;
  }

  Tensor TextEmbedder::ProcessText(std::string_view text)
  {
    auto chunks = ParseAttentionFrames(text.data());

    vector<const char*> texts;
    texts.reserve(chunks.size());
    for (auto& chunk : chunks)
    {
      texts.push_back(chunk.Text.c_str());
    }

    auto tokenizedTexts = _textTokenizer.TokenizeText(texts);
    auto [tokenizedText, attentionMask] = MergeTokenizedChunks(tokenizedTexts, chunks);
    auto encodedText = _textEncoder.EncodeText(tokenizedText);
    ApplyAttention(encodedText, attentionMask);

    return encodedText;
  }

  std::pair<Tensor, vector<float>> TextEmbedder::MergeTokenizedChunks(const Tensor& tokenizedChunks, std::span<const PromptAttentionFrame> textChunks)
  {
    Tensor tokenizedTensor{ TensorType::Int32, 1, TextTokenizer::MaxTokenCount };
    auto tokenTarget = tokenizedTensor.AsSpan<int32_t>();
    auto pTokenTarget = tokenTarget.data();
    *pTokenTarget++ = TextTokenizer::StartToken;

    vector<float> attentionMask;
    attentionMask.resize(TextTokenizer::MaxTokenCount);
    auto pAttention = attentionMask.data();
    *pAttention++ = 1;

    auto availableSpace = int32_t(TextTokenizer::MaxTokenCount) - 1;
    for (size_t i = 0; i < tokenizedChunks.Shape[0]; i++)
    {
      auto tokenizedChunk = tokenizedChunks.AsSubSpan<int32_t>(i);
      auto lastToken = tokenizedChunk.end() - 1;
      while (lastToken > tokenizedChunk.begin() && *lastToken == TextTokenizer::BlankToken) lastToken--;

      auto tokensToCopy = int32_t(distance(tokenizedChunk.begin(), lastToken));
      auto copiableLength = min(tokensToCopy, availableSpace);
            
      copy(tokenizedChunk.begin() + 1, tokenizedChunk.begin() + 1 + copiableLength, pTokenTarget);
      pTokenTarget += copiableLength;
      
      fill(pAttention, pAttention + copiableLength, textChunks[i].Attention);
      pAttention += copiableLength;

      availableSpace -= copiableLength;
    }

    fill(pTokenTarget, tokenTarget.data() + tokenTarget.size(), TextTokenizer::BlankToken);
    fill(pAttention, attentionMask.data() + attentionMask.size(), 1.f);

    return { tokenizedTensor, attentionMask };
  }

  void TextEmbedder::ApplyAttention(Tensor& encodedText, std::span<const float> attentionMask)
  {
    auto encodedTokens = encodedText.AsSpan<float>();
    auto oldAverage = accumulate(encodedTokens.begin(), encodedTokens.end(), 0.f) / encodedTokens.size();

    for (auto i = 0; i < attentionMask.size(); i++)
    {
      auto encodedToken = encodedText.AsSubSpan<float>(0, i);
      
      auto scale = attentionMask[i];
      for (auto& encodedSubtoken : encodedToken)
      {
        encodedSubtoken *= scale;
      }
    }

    auto newAverage = accumulate(encodedTokens.begin(), encodedTokens.end(), 0.f) / encodedTokens.size();
    auto compensation = oldAverage / newAverage;
    for (auto& encodedSubtoken : encodedTokens)
    {
      encodedSubtoken *= compensation;
    }
  }
}
#endif