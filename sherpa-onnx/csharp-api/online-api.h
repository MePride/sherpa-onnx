// sherpa-onnx/cpp-api/c-api.h
//
// Copyright (c)  2023  Xiaomi Corporation

// C API for sherpa-onnx
//
// Please refer to
// https://github.com/k2-fsa/sherpa-onnx/blob/master/c-api-examples/decode-file-c-api.c
// for usages.
//

#ifndef SHERPA_ONNX_CPP_API_C_API_H_
#define SHERPA_ONNX_CPP_API_C_API_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
	namespace sherpa_onnx
	{
		/// Please refer to
		/// https://k2-fsa.github.io/sherpa/onnx/pretrained_models/index.html
		/// to download pre-trained models. That is, you can find encoder-xxx.onnx
		/// decoder-xxx.onnx, joiner-xxx.onnx, and tokens.txt for this struct
		/// from there.
		typedef struct SherpaOnnxOnlineTransducer {
			const char* encoder;
			const char* decoder;
			const char* joiner;
		} SherpaOnnxOnlineTransducer;

		typedef struct SherpaOnnxOnlineModelConfig
		{
			const SherpaOnnxOnlineTransducer transducer;
			const char* tokens;
			const int32_t num_threads;
			const bool debug;  // true to print debug information of the model
		}SherpaOnnxOnlineModelConfig;

		/// It expects 16 kHz 16-bit single channel wave format.
		typedef struct SherpaOnnxFeatureConfig {
			/// Sample rate of the input data. MUST match the one expected
			/// by the model. For instance, it should be 16000 for models provided
			/// by us.
			int32_t sample_rate;

			/// Feature dimension of the model.
			/// For instance, it should be 80 for models provided by us.
			int32_t feature_dim;
		} SherpaOnnxFeatureConfig;

		typedef struct SherpaOnnxOnlineRecognizerConfig {
			SherpaOnnxFeatureConfig feat_config;
			SherpaOnnxOnlineModelConfig model_config;

			/// Possible values are: greedy_search, modified_beam_search
			const char* decoding_method;

			/// Used only when decoding_method is modified_beam_search
			/// Example value: 4
			int32_t max_active_paths;

			/// 0 to disable endpoint detection.
			/// A non-zero value to enable endpoint detection.
			int enable_endpoint;

			/// An endpoint is detected if trailing silence in seconds is larger than
			/// this value even if nothing has been decoded.
			/// Used only when enable_endpoint is not 0.
			float rule1_min_trailing_silence;

			/// An endpoint is detected if trailing silence in seconds is larger than
			/// this value after something that is not blank has been decoded.
			/// Used only when enable_endpoint is not 0.
			float rule2_min_trailing_silence;

			/// An endpoint is detected if the utterance in seconds is larger than
			/// this value.
			/// Used only when enable_endpoint is not 0.
			float rule3_min_utterance_length;
		} SherpaOnnxOnlineRecognizerConfig;

		typedef struct SherpaOnnxOnlineRecognizerResult {
			const char* text;
			int text_len;
			// TODO(fangjun): Add more fields
		} SherpaOnnxOnlineRecognizerResult;

		/// Note: OnlineRecognizer here means StreamingRecognizer.
		/// It does not need to access the Internet during recognition.
		/// Everything is run locally.
		typedef struct SherpaOnnxOnlineRecognizer SherpaOnnxOnlineRecognizer;
		typedef struct SherpaOnnxOnlineStream SherpaOnnxOnlineStream;

		/// @param config  Config for the recongizer.
		/// @return Return a pointer to the recognizer. The user has to invoke
		//          DestroyOnlineRecognizer() to free it to avoid memory leak.
		extern "C" __declspec(dllexport)
			SherpaOnnxOnlineRecognizer* __stdcall CreateOnlineRecognizer(
				const SherpaOnnxOnlineRecognizerConfig * config);

		/// Free a pointer returned by CreateOnlineRecognizer()
		///
		/// @param p A pointer returned by CreateOnlineRecognizer()
		extern "C" __declspec(dllexport)
			void __stdcall DestroyOnlineRecognizer(SherpaOnnxOnlineRecognizer* recognizer);

		/// Create an online stream for accepting wave samples.
		///
		/// @param recognizer  A pointer returned by CreateOnlineRecognizer()
		/// @return Return a pointer to an OnlineStream. The user has to invoke
		///         DestroyOnlineStream() to free it to avoid memory leak.
		extern "C" __declspec(dllexport)
			SherpaOnnxOnlineStream* __stdcall CreateOnlineStream(
				const SherpaOnnxOnlineRecognizer* recognizer);

		/// Destroy an online stream.
		///
		/// @param stream A pointer returned by CreateOnlineStream()
		extern "C" __declspec(dllexport)
			void __stdcall DestroyOnlineStream(SherpaOnnxOnlineStream* stream);

		/// Accept input audio samples and compute the features.
		/// The user has to invoke DecodeOnlineStream() to run the neural network and
		/// decoding.
		///
		/// @param stream  A pointer returned by CreateOnlineStream().
		/// @param sample_rate  Sample rate of the input samples. If it is different
		///                     from config.feat_config.sample_rate, we will do
		///                     resampling inside sherpa-onnx.
		/// @param samples A pointer to a 1-D array containing audio samples.
		///                The range of samples has to be normalized to [-1, 1].
		/// @param n  Number of elements in the samples array.
		extern "C" __declspec(dllexport)
			void __stdcall AcceptOnlineWaveform(SherpaOnnxOnlineStream* stream, int32_t sample_rate,
				const float* samples, int32_t n);

		/// Return 1 if there are enough number of feature frames for decoding.
		/// Return 0 otherwise.
		///
		/// @param recognizer  A pointer returned by CreateOnlineRecognizer
		/// @param stream  A pointer returned by CreateOnlineStream
		extern "C" __declspec(dllexport)
			int32_t __stdcall IsOnlineStreamReady(SherpaOnnxOnlineRecognizer* recognizer,
				SherpaOnnxOnlineStream* stream);

		/// Call this function to run the neural network model and decoding.
		//
		/// Precondition for this function: IsOnlineStreamReady() MUST return 1.
		///
		/// Usage example:
		///
		///  while (IsOnlineStreamReady(recognizer, stream)) {
		///     DecodeOnlineStream(recognizer, stream);
		///  }
		///
		extern "C" __declspec(dllexport)
			void __stdcall DecodeOnlineStream(SherpaOnnxOnlineRecognizer* recognizer,
				SherpaOnnxOnlineStream* stream);

		/// This function is similar to DecodeOnlineStream(). It decodes multiple
		/// OnlineStream in parallel.
		///
		/// Caution: The caller has to ensure each OnlineStream is ready, i.e.,
		/// IsOnlineStreamReady() for that stream should return 1.
		///
		/// @param recognizer  A pointer returned by CreateOnlineRecognizer()
		/// @param streams  A pointer array containing pointers returned by
		///                 CreateOnlineRecognizer()
		/// @param n  Number of elements in the given streams array.
		extern "C" __declspec(dllexport)
			void __stdcall DecodeMultipleOnlineStreams(SherpaOnnxOnlineRecognizer* recognizer,
				SherpaOnnxOnlineStream** streams, int32_t n);

		/// Get the decoding results so far for an OnlineStream.
		///
		/// @param recognizer A pointer returned by CreateOnlineRecognizer().
		/// @param stream A pointer returned by CreateOnlineStream().
		/// @return A pointer containing the result. The user has to invoke
		///         DestroyOnlineRecognizerResult() to free the returned pointer to
		///         avoid memory leak.
		extern "C" __declspec(dllexport)
			SherpaOnnxOnlineRecognizerResult* __stdcall GetOnlineStreamResult(
				SherpaOnnxOnlineRecognizer* recognizer, SherpaOnnxOnlineStream* stream);

		/// Destroy the pointer returned by GetOnlineStreamResult().
		///
		/// @param r A pointer returned by GetOnlineStreamResult()
		extern "C" __declspec(dllexport)
			void __stdcall DestroyOnlineRecognizerResult(const SherpaOnnxOnlineRecognizerResult* r);

		/// Reset an OnlineStream , which clears the neural network model state
		/// and the state for decoding.
		///
		/// @param recognizer A pointer returned by CreateOnlineRecognizer().
		/// @param stream A pointer returned by CreateOnlineStream
		extern "C" __declspec(dllexport)
			void __stdcall Reset(SherpaOnnxOnlineRecognizer* recognizer,
				SherpaOnnxOnlineStream* stream);

		/// Signal that no more audio samples would be available.
		/// After this call, you cannot call AcceptWaveform() any more.
		///
		/// @param stream A pointer returned by CreateOnlineStream()
		extern "C" __declspec(dllexport)
			void __stdcall InputFinished(SherpaOnnxOnlineStream* stream);

		/// Return 1 if an endpoint has been detected.
		///
		/// @param recognizer A pointer returned by CreateOnlineRecognizer()
		/// @param stream A pointer returned by CreateOnlineStream()
		/// @return Return 1 if an endpoint is detected. Return 0 otherwise.
		extern "C" __declspec(dllexport)
			int32_t __stdcall IsEndpoint(SherpaOnnxOnlineRecognizer* recognizer,
				SherpaOnnxOnlineStream* stream);

		// for displaying results on Linux/macOS.
		typedef struct SherpaOnnxDisplay SherpaOnnxDisplay;

		/// Create a display object. Must be freed using DestroyDisplay to avoid
		/// memory leak.
		extern "C" __declspec(dllexport)
			SherpaOnnxDisplay* __stdcall CreateDisplay(int32_t max_word_per_line);

		extern "C" __declspec(dllexport)
			void __stdcall DestroyDisplay(SherpaOnnxDisplay* display);

		/// Print the result.
		extern "C" __declspec(dllexport)
			void __stdcall SherpaOnnxPrint(SherpaOnnxDisplay* display, int32_t idx, const char* s);
	}

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif  // SHERPA_ONNX_C_API_C_API_H_
