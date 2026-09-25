package org.jellyfin.androidtv.auth.proxy

import okhttp3.Headers

/**
 * Parser for the proxy headers entered by the user. Headers are written as `Name: value` and separated by a newline or
 * a `|` character.
 */
object ProxyHeadersParser {
	private val separators = Regex("[\\n|]")

	/**
	 * Headers that can't be overwritten as they are required for the connection to Jellyfin.
	 */
	private val reservedHeaders = setOf(
		"authorization",
		"x-emby-authorization",
		"x-emby-token",
		"x-mediabrowser-token",
		"host",
		"content-length",
		"content-type",
	)

	sealed interface Result {
		data class Success(val headers: Map<String, String>) : Result
		data class InvalidHeader(val line: String) : Result
		data class ReservedHeader(val name: String) : Result
	}

	fun parse(input: String): Result {
		val headers = linkedMapOf<String, String>()

		for (line in input.split(separators).map { it.trim() }.filter { it.isNotEmpty() }) {
			val separatorIndex = line.indexOf(':')
			if (separatorIndex <= 0) return Result.InvalidHeader(line)

			val name = line.substring(0, separatorIndex).trim()
			val value = line.substring(separatorIndex + 1).trim()

			// Use OkHttp to validate the header name and value
			if (runCatching { Headers.headersOf(name, value) }.isFailure) return Result.InvalidHeader(line)
			if (name.lowercase() in reservedHeaders) return Result.ReservedHeader(name)

			headers[name] = value
		}

		return Result.Success(headers)
	}
}
