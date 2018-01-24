package gui

import (
	"github.com/skycoin/skycoin/src/cipher"
	"github.com/skycoin/skycoin/src/coin"
	"github.com/skycoin/skycoin/src/visor"
	"github.com/skycoin/skycoin/src/visor/historydb"
	"github.com/skycoin/skycoin/src/wallet"
	"github.com/skycoin/skycoin/src/daemon"
)

// Gatewayer interface for Gateway methods
type Gatewayer interface {
	Spend(wltID string, coins uint64, dest cipher.Address) (*coin.Transaction, error)
	GetWalletBalance(wltID string) (wallet.BalancePair, error)
	GetWallet(wltID string) (wallet.Wallet, error)
	UpdateWalletLabel(wltID, label string) error
	GetAddressTxns(a cipher.Address) (*visor.TransactionResults, error)
	GetWalletUnconfirmedTxns(wltID string) ([]visor.UnconfirmedTxn, error)
	GetUxOutByID(id cipher.SHA256) (*historydb.UxOut, error)
	GetUnspentOutputs(filters ...daemon.OutputsFilter) (visor.ReadableOutputSet, error)
}
