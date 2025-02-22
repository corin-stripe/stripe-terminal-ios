//
//  SCPTerminal.h
//  StripeTerminal
//
//  Created by Ben Guo on 7/28/17.
//  Copyright © 2017 Stripe. All rights reserved.
//
//  Use of this SDK is subject to the Stripe Terminal Terms:
//  https://stripe.com/terminal/legal
//

#import <Foundation/Foundation.h>

#import "SCPBlocks.h"
#import "SCPConnectionStatus.h"
#import "SCPDeviceType.h"
#import "SCPPaymentStatus.h"
#import "SCPReaderDisplayDelegate.h"
#import "SCPReaderEvent.h"
#import "SCPDiscoveryMethod.h"
#import "SCPLogLevel.h"

NS_ASSUME_NONNULL_BEGIN

/**
 The current version of this library.
 */
static NSString *const SCPSDKVersion = @"1.0.3";

@class SCPCancelable,
SCPDiscoveryConfiguration,
SCPPaymentIntentParameters,
SCPReadReusableCardParameters,
SCPUpdateReaderSoftwareParameters;

@protocol SCPConnectionTokenProvider, SCPTerminalDelegate, SCPDiscoveryDelegate, SCPReaderSoftwareUpdateDelegate;

/**
 The `SCPTerminal` object that is made available by the Stripe Terminal SDK exposes
 a generic interface for discovering readers, connecting to a reader, and
 creating payments.

 The `SCPTerminal` object is a singleton, and you cannot create multiple
 `SCPTerminal` instances.
 */
NS_SWIFT_NAME(Terminal)
@interface SCPTerminal : NSObject

/**
 Sets the token provider for the shared (singleton) `SCPTerminal` instance.
 
 You must set a token provider before calling `shared` to initialize the
 Terminal singleton. We recommend calling `setTokenProvider:` in your
 AppDelegate's `application:didFinishLaunchingWithOptions:` method.
 Alternatively, you can wrap your call to `setTokenProvider:` with a
 `dispatch_once` in Objective-C, or use a static constructor in Swift.
 
 Note that you may only set a token provider *before* requesting the shared
 Terminal instance for the first time. In order to switch accounts in your app,
 e.g. to switch between live and test Stripe API keys on your backend, refer
 to the documentation for the `clearCachedCredentials` method on the shared
 Terminal instance.
 */
+ (void)setTokenProvider:(id<SCPConnectionTokenProvider>)tokenProvider NS_SWIFT_NAME(setTokenProvider(_:));


/**
 Returns true if a token provider has been set, through `setTokenProvider:`
 */
+ (BOOL)hasTokenProvider;

/**
 Sets a block to listen for logs from the shared Terminal instance (optional).
 
 You can use this optional method to listen for logs from the Stripe Terminal
 SDK and incorporate them into your own remote logs. Note that these logs
 are subject to change, and provided for informational and debugging purposes
 only. You should not depend on them for behavior in your app. Also note that
 the block you provide may be called from any thread.

 To print internal logs from the SDK to the console, you can set `logLevel` to
 `SCPLogLevelVerbose` on the Terminal instance.
 */
+ (void)setLogListener:(SCPLogListenerBlock)listener;

/**
 Returns the shared (singleton) Terminal instance.
 
 Before accessing the singleton for the first time, you must first call
 `setTokenProvider:` and `setDelegate:`.
 */
@property (class, nonatomic, readonly) SCPTerminal *shared;

/**
 The terminal's delegate (optional).
 
 Set this to handle events from the Terminal instance.
 */
@property (nonatomic, nullable, readwrite) id<SCPTerminalDelegate> delegate;

/**
 Information about the connected `SCPReader`, or nil if no reader is connected.
 */
@property (nonatomic, nullable, readonly) SCPReader *connectedReader;

/**
 The terminal's current connection status.
 */
@property (nonatomic, readonly) SCPConnectionStatus connectionStatus;

/**
 The log level for the SDK. The default value is `.none`.
 */
@property (nonatomic, assign, readwrite) SCPLogLevel logLevel;

/**
 The terminal's current payment status.
 */
@property (nonatomic, readonly) SCPPaymentStatus paymentStatus;

/**
 Clears the current connection token, saved reader sessions, and any other
 cached credentials. You can use this method to switch accounts in your app,
 e.g. to switch between live and test Stripe API keys on your backend.
 
 In order to switch accounts in your app:
 - if a reader is connected, call `disconnectReader:`
 - configure the `tokenProvider` object to return connection tokens for the new account.
 The `tokenProvider` is implemented by your code, and you can do this however you want.
 - call `clearCachedCredentials`
 - call `discoverReaders` and `connectReader` to connect to a reader. The `connect` call
 will request a new connection token from your backend server via the token provider.
 
 An overview of the lifecycle of a connection token under the hood:
 - When the Terminal singleton is initialized, the SDK attempts to proactively request
 a connection token from your backend server.
 - When `connect` is called, the SDK uses the connection token and reader
 information to create a reader session.
 - Subsequent calls to `connect` will re-use the reader session if the SDK
 has successfully connected to the reader already. Otherwise, it will require a
 new connection token when you call `connect` again.
 */
- (void)clearCachedCredentials NS_SWIFT_NAME(clearCachedCredentials());

/**
 Begins discovering readers matching the given configuration.

 When `discoverReaders` is called, the terminal begins scanning for readers using
 the settings in the given `SCPDiscoveryConfiguration`. You must implement
 `SCPDiscoveryDelegate` to handle displaying discovery results to your user and
 connecting to a selected reader.

 The discovery process will stop on its own when the terminal successfully
 connects to a reader, if the command is canceled, or if a discovery error occurs.

 Note that if `discoverReaders` is canceled, the completion block will be called
 with `nil` (rather than an `SCPErrorCanceled` error).

 @see https://stripe.com/docs/terminal/readers/connecting

 @param configuration   The configuration for reader discovery.
 @param delegate        Your delegate for reader discovery.
 @param completion      The completion block called when the command completes.
 */
- (nullable SCPCancelable *)discoverReaders:(SCPDiscoveryConfiguration *)configuration
                                   delegate:(id<SCPDiscoveryDelegate>)delegate
                                 completion:(SCPErrorCompletionBlock)completion NS_SWIFT_NAME(discoverReaders(_:delegate:completion:));

/**
 Attempts to connect to the given reader.

 If the connect succeeds, the completion block will be called with the
 connected reader, and the terminal's `connectionStatus` will change to
 `.connected`.

 If the connect fails, the completion block will be called with an error.

 The terminal must be actively discovering readers in order to connect to one.
 The discovery process will stop if this connection request succeeds, otherwise
 the terminal will continue discovering.

 Under the hood, the SDK uses the `fetchConnectionToken` method you defined
 to fetch a connection token if it does not already have one. It then uses the
 connection token and reader information to create a reader session.

 @see https://stripe.com/docs/terminal/readers/connecting

 @param reader          The reader to connect to. This should be a reader
 recently returned to the `didUpdateDiscoveredReaders:` method.
 @param completion      The completion block called when the command completes.
 */
- (void)connectReader:(SCPReader *)reader completion:(SCPReaderCompletionBlock)completion NS_SWIFT_NAME(connectReader(_:completion:));

/**
 Attempts to disconnect from the currently connected reader.
 
 If the disconnect succeeds, the completion block is called with `nil`. If the
 disconnect fails, the completion block is called with an error.

 @see https://stripe.com/docs/terminal/readers/connecting
 
 @param completion      The completion block called when the command completes.
 */
- (void)disconnectReader:(SCPErrorCompletionBlock)completion NS_SWIFT_NAME(disconnectReader(_:));

/**
 Creates a new `SCPPaymentIntent` with the given parameters.
 
 Note: If the information required to create a PaymentIntent isn't readily
 available in your app, you can create the PaymentIntent on your server and use
 the `retrievePaymentIntent` method to retrieve the PaymentIntent in your app.

 @see https://stripe.com/docs/terminal/payments#create

 @param parameters      The parameters for the PaymentIntent to be created.
 @param completion      The completion block called when the command completes.
 */
- (void)createPaymentIntent:(SCPPaymentIntentParameters *)parameters
                 completion:(SCPPaymentIntentCompletionBlock)completion NS_SWIFT_NAME(createPaymentIntent(_:completion:));

/**
 Retrieves a `SCPPaymentIntent` with a client secret.
 
 If the information required to create a PaymentIntent isn't readily available
 in your app, you can create the PaymentIntent on your server and use this
 method to retrieve the PaymentIntent in your app.
 
 @see https://stripe.com/docs/terminal/payments#create
 
 @param clientSecret    The client secret of the PaymentIntent to be retrieved.
 @param completion      The completion block called when the command completes.
 */
- (void)retrievePaymentIntent:(NSString *)clientSecret
                   completion:(SCPPaymentIntentCompletionBlock)completion NS_SWIFT_NAME(retrievePaymentIntent(clientSecret:completion:));

/**
 Collects a payment method for the given `SCPPaymentIntent`.

 Note: `collectPaymentMethod` does not apply any changes to the PaymentIntent
 API object. Updates to the PaymentIntent are local to the SDK, and persisted
 in-memory.

 If collecting a payment method fails, the completion block will be called with
 an error. After resolving the error, you may call `collectPaymentMethod` again
 to either try the same card again, or try a different card.

 If collecting a payment method succeeds, the completion block will be called
 with a PaymentIntent with status `.requiresConfirmation`, indicating that you
 should call `processPayment:completion:` to finish the payment.

 Note that if `collectPaymentMethod` is canceled, the completion block will be
 called with a `Canceled` error.

 @see https://stripe.com/docs/terminal/payments#collect

 @param paymentIntent       The PaymentIntent to collect a payment method for.

 @param delegate            Your delegate for handling reader input events.
 Providing an `SCPReaderDisplayDelegate` is required when the connected reader doesn't
 have a built-in display (e.g., the BBPOS Chipper 2X BT). These readers will
 request that your app display messages during the `collectPaymentMethod`
 process (e.g., "Remove card") via this delegate.

 @param completion          The completion block called when the command completes.
 */
- (nullable SCPCancelable *)collectPaymentMethod:(SCPPaymentIntent *)paymentIntent
                                        delegate:(nullable id<SCPReaderDisplayDelegate>)delegate
                                      completion:(SCPPaymentIntentCompletionBlock)completion NS_SWIFT_NAME(collectPaymentMethod(_:delegate:completion:));

/**
 Processes a payment after collecting a payment method succeeds.

 Synchronous capture
 -------------------

 Stripe Terminal uses two-step card payments to prevent unintended and duplicate
 payments. When `processPayment` completes successfully, a charge has been
 authorized on the customer's card, but not yet been "captured". Your app must
 **synchronously notify your backend** to capture the PaymentIntent
 in order to settle the funds to your account.

 Handling failures
 -----------------

 When `processPayment` fails, the SDK returns an error that includes the
 updated `SCPPaymentIntent`. Your app should inspect the updated PaymentIntent
 to decide how to retry the payment.

 1. If the updated PaymentIntent is `nil`, the request to Stripe's servers timed
 out and the PaymentIntent's status is unknown. We recommend that you retry
 `processPayment` with the original PaymentIntent. If you instead choose to
 abandon the original PaymentIntent and create a new one, **do not capture**
 the original PaymentIntent. If you do, you might charge your customer twice.

 2. If the updated PaymentIntent's status is still `.requiresConfirmation` (e.g.,
 the request failed because your app is not connected to the internet), you
 can call `processPayment` again with the updated PaymentIntent to retry
 the request.

 3. If the updated PaymentIntent's status changes to `.requiresPaymentMethod` (e.g., the
 request failed because the card was declined), call `collectPaymentMethod` with the
 updated PaymentIntent to try charging another card.

 @see https://stripe.com/docs/terminal/payments#process

 @param paymentIntent   The PaymentIntent to confirm.
 @param completion      The completion block called when the confirm completes.
 */
- (void)processPayment:(SCPPaymentIntent *)paymentIntent
            completion:(SCPProcessPaymentCompletionBlock)completion NS_SWIFT_NAME(processPayment(_:completion:));

/**
 Cancels an `SCPPaymentIntent`.
 
 If the cancel request succeeds, the completion block will be called with the
 updated PaymentIntent object with status Canceled. If the cancel request
 fails, the completion block will be called with an error.

 @see https://stripe.com/docs/terminal/payments/refunds
 
 @param paymentIntent     The PaymentIntent to cancel.
 @param completion        The completion block called when the cancel completes.
 */
- (void)cancelPaymentIntent:(SCPPaymentIntent *)paymentIntent
                 completion:(SCPPaymentIntentCompletionBlock)completion NS_SWIFT_NAME(cancelPaymentIntent(_:completion:));

/**
 Reads a card with the given parameters and returns a PaymentMethod.

 **NOTE: Most integrations should not use `readReusableCard`.**

 You should create a `SCPPaymentIntent` and use the associated `collectPaymentMethod`
 and `processPayment` methods if you are simply collecting a payment from
 a customer.

 You can use `readReusableCard` to read payment details and defer payment for later.
 The PaymentMethod created by this method will have type `card`, suitable for
 use with online payments.

 Note that if you use this method to defer a payment, the transaction will
 *not* receive the beneficial rates and liability shift associated with card
 present transactions.

 If reading a card fails, the completion block will be called with an error
 containing details about the failure. If reading a card succeeds, the
 completion block will be called with a `PaymentMethod`. You should send
 the ID of the payment method to your backend for further processing. For example,
 you can use the fingerprint to look up charges created using the same
 card.

 @see https://stripe.com/docs/terminal/online-payments

 @param parameters  The parameters for reading the card.
 @param delegate    Your delegate for handling reader input events.
 @param completion  The completion block called when the command completes.
 */
- (nullable SCPCancelable *)readReusableCard:(SCPReadReusableCardParameters *)parameters
                                    delegate:(id<SCPReaderDisplayDelegate>)delegate
                                  completion:(SCPPaymentMethodCompletionBlock)completion NS_SWIFT_NAME(readReusableCard(_:delegate:completion:));

/**
 Checks for a reader software update, and returns an update object if an update
 is available.

 If an update is available, the completion block will be called with an
 `SCPReaderSoftwareUpdate` object, indicating that a software update is available.
 The update object contains information about the update that you can display
 to the user, such as the estimated installation time.

 If an error occurs while checking for an update, the completion block will be
 called with an error. If there are no updates available and no errors occur,
 the completion block will be called with `(nil, nil)`.

 @see https://stripe.com/docs/terminal/readers/bbpos-chipper2xbt#software-updates-and-releases

 @param completion  The completion block called when checking for an update
 completes.
 */
- (nullable SCPCancelable *)checkForUpdate:(SCPReaderSoftwareUpdateCompletionBlock)completion NS_SWIFT_NAME(checkForUpdate(_:));

/**
 Installs the provided update, and calls the completion block when the
 installation has completed.

 If an error occurs while installing the update (e.g. because the update was
 interrupted), the completion block will be called with an error. If the update
 completed successfully, the completion block will be called with `nil`.
 You must pass a delegate to handle events as the update proceeds. In your app,
 you should display the progress of the update to the user. You should also
 instruct the user to wait for the update to complete: "Do not leave this page,
 and keep the reader in range and powered on until the update is complete."

 You must implement the ability to update your reader's software in your app.
 Though we expect required software updates to be very rare, by using Stripe
 Terminal, you are obligated to include this functionality.

 @see https://stripe.com/docs/terminal/readers/bbpos-chipper2xbt#software-updates-and-releases

 @param update  The reader update.
 @param delegate  Your delegate for handling update events.
 @param completion  The completion block called when the update completes.
 */
- (nullable SCPCancelable *)installUpdate:(SCPReaderSoftwareUpdate *)update
                                 delegate:(id<SCPReaderSoftwareUpdateDelegate>)delegate
                               completion:(SCPErrorCompletionBlock)completion NS_SWIFT_NAME(installUpdate(_:delegate:completion:));

/**
 Returns an unlocalized string for the given reader input options, e.g.
 "Swipe / Insert"
 */
+ (NSString *)stringFromReaderInputOptions:(SCPReaderInputOptions)options NS_SWIFT_NAME(stringFromReaderInputOptions(_:));

/**
 Returns an unlocalized string for the given reader display message, e.g.
 "Retry Card"
 */
+ (NSString *)stringFromReaderDisplayMessage:(SCPReaderDisplayMessage)message NS_SWIFT_NAME(stringFromReaderDisplayMessage(_:));

/**
 Returns an unlocalized string for the given reader event, e.g.
 "Card Inserted"
 */
+ (NSString *)stringFromReaderEvent:(SCPReaderEvent)event NS_SWIFT_NAME(stringFromReaderEvent(_:));

/**
 Returns an unlocalized string for the given connection status, e.g.
 "Connecting"
 */
+ (NSString *)stringFromConnectionStatus:(SCPConnectionStatus)status NS_SWIFT_NAME(stringFromConnectionStatus(_:));

/**
 Returns an unlocalized string for the given payment status, e.g.
 "Not Ready"
 */
+ (NSString *)stringFromPaymentStatus:(SCPPaymentStatus)status NS_SWIFT_NAME(stringFromPaymentStatus(_:));

/**
 Returns an unlocalized string for the given device type.
 */
+ (NSString *)stringFromDeviceType:(SCPDeviceType)deviceType NS_SWIFT_NAME(stringFromDeviceType(_:));

/**
 Returns an unlocalized string for the given discovery method.
 */
+ (NSString *)stringFromDiscoveryMethod:(SCPDiscoveryMethod)method NS_SWIFT_NAME(stringFromDiscoveryMethod(_:));

/**
 Use `initWithConfiguration:tokenProvider:delegate:`
 */
- (instancetype)init NS_UNAVAILABLE;

/**
 Use `initWithConfiguration:tokenProvider:delegate:`
 */
- (instancetype)new NS_UNAVAILABLE;

@end

NS_ASSUME_NONNULL_END
