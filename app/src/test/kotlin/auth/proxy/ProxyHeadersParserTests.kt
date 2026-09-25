package org.jellyfin.androidtv.auth.proxy

import io.kotest.core.spec.style.FunSpec
import io.kotest.matchers.shouldBe

class ProxyHeadersParserTests : FunSpec({
	test("Empty input returns no headers") {
		ProxyHeadersParser.parse("") shouldBe ProxyHeadersParser.Result.Success(emptyMap())
		ProxyHeadersParser.parse("  |  \n ") shouldBe ProxyHeadersParser.Result.Success(emptyMap())
	}

	test("Headers are split on pipes and newlines") {
		ProxyHeadersParser.parse("P-Access-Token-Id: abc | P-Access-Token: def:ghi\nX-Custom:1") shouldBe
			ProxyHeadersParser.Result.Success(
				mapOf(
					"P-Access-Token-Id" to "abc",
					"P-Access-Token" to "def:ghi",
					"X-Custom" to "1",
				)
			)
	}

	test("Invalid headers are rejected") {
		ProxyHeadersParser.parse("no separator") shouldBe ProxyHeadersParser.Result.InvalidHeader("no separator")
		ProxyHeadersParser.parse(": value") shouldBe ProxyHeadersParser.Result.InvalidHeader(": value")
		ProxyHeadersParser.parse("Bad Name: value") shouldBe ProxyHeadersParser.Result.InvalidHeader("Bad Name: value")
	}

	test("Reserved headers are rejected") {
		ProxyHeadersParser.parse("authorization: Basic abc") shouldBe ProxyHeadersParser.Result.ReservedHeader("authorization")
		ProxyHeadersParser.parse("X-Emby-Token: abc") shouldBe ProxyHeadersParser.Result.ReservedHeader("X-Emby-Token")
	}
})
