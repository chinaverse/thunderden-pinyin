#pragma once

#include "review.h"
#include "transport.h"
#include <memory>

namespace td {
using WalletApproval = std::function<bool(const ReviewLines&, const ReviewLines&)>;
std::optional<Digest> ApproveWallet(const Policy& policy, const Keys& keys, const WalletApproval& approve);
class Terminal;
// Returns no keys when the user chooses to change network before loading a wallet.
// `chinese` is Terminal::ChineseInput; an empty function hides Chinese entry.
std::unique_ptr<Keys> StartSession(Terminal& terminal, const std::function<QRMessage()>& scan,
    const std::function<SecretBytes(Terminal&, size_t)>& chinese = {});
bool EndSessionForSeed(Terminal& terminal, const QRMessage& message);
bool LoadWallet(Terminal& terminal, const Keys& keys, std::optional<ApprovedWallet>& loaded, const QRMessage& message);
bool LoadWallet(Terminal& terminal, const Keys& keys, std::optional<ApprovedWallet>& loaded, Policy policy, bool from_psbt);
bool ApproveTransaction(Terminal& terminal, const TransactionReview& review, std::optional<ReviewScreen>& completed);
std::optional<QRMessage> SignRequest(Terminal& terminal, const Keys& keys, std::optional<ApprovedWallet>& loaded,
    const QRMessage& message, const std::function<QRMessage()>& scan,
    const std::function<bool(const TransactionReview&)>& approve);
}
