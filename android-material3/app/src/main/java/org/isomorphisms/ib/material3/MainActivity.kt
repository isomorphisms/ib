package org.isomorphisms.ib.material3

import android.content.res.AssetManager
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.weight
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.selection.SelectionContainer
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.ListItem
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBar
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.unit.dp
import java.io.IOException

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val articles = loadPensieveArticles(assets)
        setContent {
            MaterialTheme {
                ArxivViewport(articles)
            }
        }
    }
}

private data class PensieveArticle(
    val arxivId: String,
    val title: String,
    val body: String?,
    val textSource: String?,
)

private fun readAssetOrNull(assets: AssetManager, path: String): String? =
    try {
        assets.open(path).bufferedReader().use { it.readText() }
    } catch (_: IOException) {
        null
    }

private fun loadPensieveArticles(assets: AssetManager): List<PensieveArticle> {
    val root = "pensieve/arxiv"
    val identifiers = assets.list(root)
        ?.filter { it.isNotBlank() }
        ?.sorted()
        .orEmpty()

    return identifiers.map { identifier ->
        val itemRoot = "$root/$identifier"
        val title = readAssetOrNull(assets, "$itemRoot/title")
            ?.trim()
            ?.takeIf { it.isNotEmpty() }
            ?: "arXiv $identifier"

        val textCandidates = listOf(
            "text/from-pdf.txt",
            "text/from-html.txt",
            "text/from-abstract.txt",
        )

        var body: String? = null
        var textSource: String? = null
        for (candidate in textCandidates) {
            val text = readAssetOrNull(assets, "$itemRoot/$candidate")
                ?.trim()
                ?.takeIf { it.isNotEmpty() }
            if (text != null) {
                body = text
                textSource = candidate
                break
            }
        }

        PensieveArticle(
            arxivId = identifier,
            title = title,
            body = body,
            textSource = textSource,
        )
    }
}

@Composable
private fun ArxivViewport(articles: List<PensieveArticle>) {
    var selected by remember(articles) { mutableStateOf(articles.firstOrNull()) }

    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("IB") },
            )
        },
    ) { scaffoldPadding ->
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(scaffoldPadding),
        ) {
            Text(
                text = "Pensieve · ${articles.size} arXiv entries",
                style = MaterialTheme.typography.labelLarge,
                modifier = Modifier.padding(horizontal = 16.dp, vertical = 8.dp),
            )

            LazyColumn(
                modifier = Modifier
                    .fillMaxWidth()
                    .heightIn(max = 260.dp),
            ) {
                items(
                    items = articles,
                    key = { article -> article.arxivId },
                ) { article ->
                    ListItem(
                        headlineContent = { Text(article.title) },
                        supportingContent = {
                            Text("Pensieve · arxiv/${article.arxivId}")
                        },
                        modifier = Modifier.clickable { selected = article },
                    )
                    HorizontalDivider()
                }
            }

            Surface(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f)
                    .padding(12.dp),
                shape = MaterialTheme.shapes.large,
                tonalElevation = 1.dp,
            ) {
                SelectionContainer {
                    Column(
                        modifier = Modifier
                            .fillMaxSize()
                            .verticalScroll(rememberScrollState())
                            .padding(18.dp),
                    ) {
                        val article = selected
                        if (article == null) {
                            Text(
                                text = "The Pensieve snapshot is empty.",
                                style = MaterialTheme.typography.bodyLarge,
                            )
                        } else {
                            Text(
                                text = article.title,
                                style = MaterialTheme.typography.headlineSmall,
                            )
                            Text(
                                text = "arXiv ${article.arxivId}",
                                style = MaterialTheme.typography.bodyMedium,
                            )
                            Text(
                                text = article.textSource?.let {
                                    "Pensieve · arxiv/${article.arxivId}/$it"
                                } ?: "Pensieve · arxiv/${article.arxivId}",
                                style = MaterialTheme.typography.labelMedium,
                            )
                            Spacer(modifier = Modifier.height(18.dp))
                            Text(
                                text = article.body
                                    ?: "No text representation is present in this frozen Pensieve snapshot.",
                                style = MaterialTheme.typography.bodyLarge,
                            )
                        }
                    }
                }
            }
        }
    }
}
